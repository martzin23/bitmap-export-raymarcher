#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define XBASE 16
#define YBASE 9
#define PI 3.14159

struct rayData {
	float position[3];
	float orientation[3];
	float hitNormal[3];
	float hitPos[3];
	float reflectionDir[3];
	float lightDir[3];
	float distanceTravelled;
	_Bool didHit;
	int hitIndex;
	int jumpCount;
} ray;
struct bodyData {
	int shapeType;
	int color[3];
	float position[3];
	float size[3];
} *body;
struct renderData {
	float diffuseColor[3];
	float diffuseLight;
	float specularLight;
	float ambientOcclusion;
	_Bool emissionMask;
	_Bool shadowMask;
	_Bool skyMask;
	float combined[3];
} render;
struct sceneSettings {
	float cameraRotation[3];
	float cameraPosition[3];
	float pixelDistance;
	float epsilon;
	int ambientLight[3];
	int resolutionMulti;
	int maxMarches;
	int maxDistance;
	int bodyCount;
	int maxReflections;
	int frameCount;
	int frameMin;
	int frameMax;
	int frameStep;
} settings;

float rayMarch();
float getDistance(float px, float py, float pz, int bodyIndex);
void vectorSubtract(float v1[], float v2[], float r1[]);
void rotateVector(float v1[], float r1[], float yaw, float pitch, float roll);	// doesn't work properly
void normalise(float v1[], float r1[]);
float dotProduct(float v1[], float v2[]);
float lerp(float x, float xmin, float xmax, float ymin, float ymax);
float clamp(float x, float min, float max);

int main() {
	// parsing input file
	if (scanf("resolutionMulti: %d\nmaxDistance: %d\nmaxMarches: %d\nmaxReflections: %d\nepsilon: %f\nambientLight: %d %d %d\n\nframeMin: %d\nframeMax: %d\nframeStep: %d\n\nfieldOfView: %f\ncameraPos: %f %f %f\ncameraRot: %f %f\n\nbodyCount: %d",
	&settings.resolutionMulti, &settings.maxDistance, &settings.maxMarches, &settings.maxReflections, &settings.epsilon, &settings.ambientLight[0], &settings.ambientLight[1], &settings.ambientLight[2], &settings.frameMin, &settings.frameMax, &settings.frameStep, &settings.pixelDistance, &settings.cameraPosition[0], &settings.cameraPosition[1], &settings.cameraPosition[2], &settings.cameraRotation[0], &settings.cameraRotation[1], &settings.bodyCount) != 18) {
		printf("Error: invalid input file");
		return 0;
	}
	body = malloc(settings.bodyCount*sizeof(struct bodyData));
	for (int i = 0; i < settings.bodyCount; i++) {
		if (scanf("\n\nbodyType: %d\nbodyColor: %d %d %d\nbodyPos: %f %f %f\nbodySize: %f %f %f",
		&body[i].shapeType, &body[i].color[0], &body[i].color[1], &body[i].color[2], &body[i].position[0], &body[i].position[1], &body[i].position[2], &body[i].size[0], &body[i].size[1], &body[i].size[2]) != 10) {
			printf("Error: invalid input file");
			return 0;
		}
	}
	printf("Started rendering...\n");
	
	// init bmp image
	unsigned int imageWidth = settings.resolutionMulti * XBASE;
	unsigned int imageHeight = settings.resolutionMulti * YBASE;
	unsigned short imagePixelBits = 24;
	const unsigned int imageBiSize = 40;
	const unsigned int imageHeaderSize = 54;
	const unsigned short imagePlains = 1;
	unsigned int imageSize = (((imageWidth * imagePixelBits + 31) / 32) * 4) * imageHeight;
	unsigned int imageFilesize = 54 + imageSize;
	unsigned char imageHeader[54] = {0};
	unsigned char *imageBody = malloc(imageSize);
	char imageName[] = "render_000.bmp"; 
	char numberSufix[3];
	memcpy(imageHeader, "BM", 2);
	memcpy(imageHeader + 2, &imageFilesize, 4);
	memcpy(imageHeader + 10, &imageHeaderSize, 4);
	memcpy(imageHeader + 14, &imageBiSize, 4);
	memcpy(imageHeader + 18, &imageWidth, 4);
	memcpy(imageHeader + 22, &imageHeight, 4);
	memcpy(imageHeader + 26, &imagePlains, 2);
	memcpy(imageHeader + 28, &imagePixelBits, 2);
	memcpy(imageHeader + 34, &imageSize, 4);

	for (settings.frameCount = settings.frameMin; settings.frameCount <= settings.frameMax; settings.frameCount += settings.frameStep) {

		// calculating pixel colors
		for (int y = 0; y < YBASE * settings.resolutionMulti; y++) {
			for (int x = 0; x < XBASE * settings.resolutionMulti; x++) {
				render.diffuseLight = 1;
				for (int reflectionCount = 0; reflectionCount == 0 || (render.diffuseColor[0] == 0 && render.diffuseColor[1] == 0 && render.diffuseColor[2] == 0 && reflectionCount <= settings.maxReflections); reflectionCount++) {

					if ( reflectionCount == 0 ) {
						// init view ray
						ray.position[0] = settings.cameraPosition[0];
						ray.position[1] = settings.cameraPosition[1];
						ray.position[2] = settings.cameraPosition[2];
						ray.orientation[0] = -settings.pixelDistance / settings.resolutionMulti * ((float)x - (XBASE * settings.resolutionMulti) / 2);
						ray.orientation[1] = -1;
						ray.orientation[2] = settings.pixelDistance / settings.resolutionMulti * ((float)y - (YBASE * settings.resolutionMulti) / 2);
						normalise(ray.orientation, ray.orientation);
						rotateVector(ray.orientation, ray.orientation, settings.cameraRotation[0], settings.cameraRotation[1], 0);
					} else {
						// init reflection ray
						ray.position[0] = ray.hitPos[0];
						ray.position[1] = ray.hitPos[1];
						ray.position[2] = ray.hitPos[2];
						ray.orientation[0] = ray.reflectionDir[0];
						ray.orientation[1] = ray.reflectionDir[1];
						ray.orientation[2] = ray.reflectionDir[2];
						ray.position[0] += 20 * settings.epsilon * ray.orientation[0];
						ray.position[1] += 20 * settings.epsilon * ray.orientation[1];
						ray.position[2] += 20 * settings.epsilon * ray.orientation[2];
					}

					// view ray
					ray.hitIndex = rayMarch();
					ray.hitPos[0] = ray.position[0];
					ray.hitPos[1] = ray.position[1];
					ray.hitPos[2] = ray.position[2];
					if (ray.didHit) {
						render.skyMask = 1;
						// normal vector https://computergraphics.stackexchange.com/questions/8093/how-to-compute-normal-of-surface-from-implicit-equation-for-ray-marching
						ray.hitNormal[0] = (getDistance(ray.position[0] + (settings.epsilon / 10), ray.position[1], ray.position[2], ray.hitIndex) - getDistance(ray.position[0], ray.position[1], ray.position[2], ray.hitIndex)) / (settings.epsilon / 10);
						ray.hitNormal[1] = (getDistance(ray.position[0], ray.position[1] + (settings.epsilon / 10), ray.position[2], ray.hitIndex) - getDistance(ray.position[0], ray.position[1], ray.position[2], ray.hitIndex)) / (settings.epsilon / 10);
						ray.hitNormal[2] = (getDistance(ray.position[0], ray.position[1], ray.position[2] + (settings.epsilon / 10), ray.hitIndex) - getDistance(ray.position[0], ray.position[1], ray.position[2], ray.hitIndex)) / (settings.epsilon / 10);

						// reflection vector https://math.stackexchange.com/questions/13261/how-to-get-a-reflection-vector
						ray.reflectionDir[0] = ray.orientation[0] - 2.0 * ((ray.orientation[0] * ray.hitNormal[0]) + (ray.orientation[1] * ray.hitNormal[1]) + (ray.orientation[2] * ray.hitNormal[2])) * ray.hitNormal[0];
						ray.reflectionDir[1] = ray.orientation[1] - 2.0 * ((ray.orientation[0] * ray.hitNormal[0]) + (ray.orientation[1] * ray.hitNormal[1]) + (ray.orientation[2] * ray.hitNormal[2])) * ray.hitNormal[1];
						ray.reflectionDir[2] = ray.orientation[2] - 2.0 * ((ray.orientation[0] * ray.hitNormal[0]) + (ray.orientation[1] * ray.hitNormal[1]) + (ray.orientation[2] * ray.hitNormal[2])) * ray.hitNormal[2];

						render.diffuseColor[0] = body[ray.hitIndex].color[0] / 255.0;
						render.diffuseColor[1] = body[ray.hitIndex].color[1] / 255.0;
						render.diffuseColor[2] = body[ray.hitIndex].color[2] / 255.0;

						// shading
						vectorSubtract(body[0].position, ray.position, ray.lightDir);
						normalise(ray.lightDir, ray.lightDir);
						render.diffuseLight *= clamp(dotProduct(ray.hitNormal, ray.lightDir), 0, 1); // diffuse lighting
						render.specularLight = pow(clamp(dotProduct(ray.reflectionDir, ray.lightDir), 0, 1), 10)*0.3; // specular reflections
						render.ambientOcclusion = clamp(lerp(ray.jumpCount, 0, settings.maxMarches, 1, 0) + pow(lerp(ray.distanceTravelled, 0, settings.maxDistance, 0, 1), 0.2), 0, 1); // ambient occlusion
						
						// light source exception
						if (ray.hitIndex == 0) render.emissionMask = 1;
						else render.emissionMask = 0;

						// shadow ray
						ray.position[0] += 20 * settings.epsilon * ray.lightDir[0];
						ray.position[1] += 20 * settings.epsilon * ray.lightDir[1];
						ray.position[2] += 20 * settings.epsilon * ray.lightDir[2];
						ray.orientation[0] = ray.lightDir[0];
						ray.orientation[1] = ray.lightDir[1];
						ray.orientation[2] = ray.lightDir[2];
						ray.hitIndex = rayMarch();
						if (ray.hitIndex != 0 && ray.didHit ) render.shadowMask = 0;
						else render.shadowMask = 1;

					} else if (reflectionCount == 0) render.skyMask = 0;
				}

				// combining values to final color
				if (render.skyMask) {
					render.combined[0] = render.diffuseColor[0] * render.ambientOcclusion * (render.diffuseLight * render.shadowMask * (body[0].color[0] / 255.0) + (settings.ambientLight[0] / 255.0)) + render.emissionMask * render.diffuseColor[0] + render.specularLight * render.shadowMask * (body[0].color[0] / 255.0);
					render.combined[1] = render.diffuseColor[1] * render.ambientOcclusion * (render.diffuseLight * render.shadowMask * (body[0].color[1] / 255.0) + (settings.ambientLight[1] / 255.0)) + render.emissionMask * render.diffuseColor[1] + render.specularLight * render.shadowMask * (body[0].color[1] / 255.0);
					render.combined[2] = render.diffuseColor[2] * render.ambientOcclusion * (render.diffuseLight * render.shadowMask * (body[0].color[2] / 255.0) + (settings.ambientLight[2] / 255.0)) + render.emissionMask * render.diffuseColor[2] + render.specularLight * render.shadowMask * (body[0].color[2] / 255.0);
				} else {
					render.combined[0] = settings.ambientLight[0] / 255.0;
					render.combined[1] = settings.ambientLight[1] / 255.0;
					render.combined[2] = settings.ambientLight[2] / 255.0;
				}

				// save pixel to image
				imageBody[y * (((imageWidth * imagePixelBits + 31) / 32) * 4) + x * 3 + 0] = (unsigned char)(clamp(render.combined[2], 0, 1) * 255);
				imageBody[y * (((imageWidth * imagePixelBits + 31) / 32) * 4) + x * 3 + 1] = (unsigned char)(clamp(render.combined[1], 0, 1) * 255);
				imageBody[y * (((imageWidth * imagePixelBits + 31) / 32) * 4) + x * 3 + 2] = (unsigned char)(clamp(render.combined[0], 0, 1) * 255);
			}
		}

		// export image
		numberSufix[0] = settings.frameCount%10 + '0';
		numberSufix[1] = settings.frameCount/10%10 + '0';
		numberSufix[2] = settings.frameCount/100 + '0';
		memcpy(imageName + 9, &numberSufix[0], 1);
		memcpy(imageName + 8, &numberSufix[1], 1);
		memcpy(imageName + 7, &numberSufix[2], 1);
		FILE *fout = fopen(imageName, "wb");
		fwrite(imageHeader, 1, 54, fout);
		fwrite(imageBody, 1, imageSize, fout);
		fclose(fout);
		printf("Finished frame #%d\n", settings.frameCount);
	}
	printf("Done!\n");
	return 0;
}

float rayMarch() {
	int bodyIndex, closestIndex;
	float closestDistance = 0;
	ray.didHit = 0;
	ray.jumpCount = ray.distanceTravelled = 0;

	for (int marchCount = 0; marchCount < settings.maxMarches && !ray.didHit && ray.distanceTravelled < settings.maxDistance; marchCount++) {
		closestIndex = 0;
		closestDistance = getDistance(ray.position[0], ray.position[1], ray.position[2], 0);
		for (bodyIndex = 1; bodyIndex < settings.bodyCount; bodyIndex++) {
			if (closestDistance > getDistance(ray.position[0], ray.position[1], ray.position[2], bodyIndex)) {
				closestDistance = getDistance(ray.position[0], ray.position[1], ray.position[2], bodyIndex);
				closestIndex = bodyIndex;
			}
		}

		if (closestDistance < settings.epsilon) ray.didHit = 1;
		else {
			ray.position[0] = ray.position[0] + closestDistance * ray.orientation[0];
			ray.position[1] = ray.position[1] + closestDistance * ray.orientation[1];
			ray.position[2] = ray.position[2] + closestDistance * ray.orientation[2];
			ray.distanceTravelled = ray.distanceTravelled + closestDistance;
			ray.jumpCount++;
		}
	}
	return closestIndex;
}

float getDistance(float x, float y, float z, int bodyIndex) {
	x -= body[bodyIndex].position[0];
	y -= body[bodyIndex].position[1];
	z -= body[bodyIndex].position[2];
	float d1, d2;
	switch (body[bodyIndex].shapeType) {
		case 0: // sphere SDF
			return sqrt(pow(x, 2) + pow(y, 2) + (pow(z, 2))) - body[bodyIndex].size[0];
		case 1: // box SDF by Inigo Quilez https://iquilezles.org/articles/distfunctions/
			return sqrt(pow(fmaxf(fabsf(x) - body[bodyIndex].size[0], 0.0), 2) + pow(fmaxf(fabsf(y) - body[bodyIndex].size[1], 0.0), 2) + pow(fmaxf(fabsf(z) - body[bodyIndex].size[2], 0.0), 2));
		case 2: // plane
			return x*body[bodyIndex].size[0] + y*body[bodyIndex].size[1] + z*body[bodyIndex].size[2];
		case 3: // ???
			return sqrt(pow(x + 0.5 * (int)(2 * sin(5 * z)), 2) + pow(y, 2) + (pow(z + 0.5 * (int)(2 * sin(5 * x)), 2))) - body[bodyIndex].size[0];
		default:
			printf("Error: unknown body type\n");
			return 0;
	}
}

void vectorSubtract(float v1[], float v2[], float r1[]) {
	r1[0] = v1[0] - v2[0];
	r1[1] = v1[1] - v2[1];
	r1[2] = v1[2] - v2[2];
	return;
}

float dotProduct(float v1[], float v2[]) {
	return (v1[0] * v2[0]) + (v1[1] * v2[1]) + (v1[2] * v2[2]);
}

void normalise(float v1[], float r1[]) {
	float sum = pow(v1[0], 2) + pow(v1[1], 2) + pow(v1[2], 2);
	r1[0] = v1[0] / sqrt(sum);
	r1[1] = v1[1] / sqrt(sum);
	r1[2] = v1[2] / sqrt(sum);
	return;
}

float lerp(float x, float xmin, float xmax, float ymin, float ymax) {
	return ((x - xmin) / (xmax - xmin)) * (ymax - ymin) + ymin;
}

void rotateVector(float v1[], float r1[], float yaw, float pitch, float roll) {
	r1[0] = v1[0] * cos(roll * (PI / 180)) + v1[2] * sin(roll * (PI / 180));
	r1[2] = -r1[0] * sin(roll * (PI / 180)) + v1[2] * cos(roll * (PI / 180));
	r1[1] = v1[1] * cos(pitch * (PI / 180)) - r1[2] * sin(pitch * (PI / 180));
	r1[2] = r1[1] * sin(pitch * (PI / 180)) + r1[2] * cos(pitch * (PI / 180));
	r1[0] = r1[0] * cos(yaw * (PI / 180)) - r1[1] * sin(yaw * (PI / 180));
	r1[1] = r1[0] * sin(yaw * (PI / 180)) + r1[1] * cos(yaw * (PI / 180));
	return;
}

float clamp(float x, float min, float max) {
	if (x < min) return min;
	else if (x > max) return max;
	else return x;
}
