#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<string.h>
#include"fft.h"
#include"hmath.h"
#include"sigProcess.h"
#include"WAVE.h"

typedef struct {
	int startSampleNum;
	int endSampleNum;
	struct activeSeg* link;
}activeSeg;

double frameSizeInTime = 0.025; 
double frameShiftSizeInTime = 0.010;
double MH = (double)1e4, ML = (double)2e3, Zs = 0.15;
double gapLeast = 0.5;
double genderDectectionWindow = 0.2;


int main(int argc, char** argv) {
	printf("%s\n", argv[0]);
	printf("This program is for VAD. Input the audio to be detected and the dir to store the transformed audio.\n ");
	printf("usage: VAD.exe <intput WAVE file> <output dir>\n");

	//open WAVE and read
	printf("Open WAVE file\n");
	WAVE_t w = initWAVE_t();
	FILE* fWAVE = NULL;
	if (argc >= 2) { fWAVE = fopen(argv[1], "rb"); printf("\nWAVE information:\nname: %s\n", argv[1]);}
	else { fWAVE = fopen("PHONE_001.wav", "rb"); printf("\nWAVE information:\nname: PHONE_001.wav\n");}
	loadWAVEFile(&w, fWAVE);
	print_WAVE(w);

	printf("\nConfiguration\n");
	//Frame Signal
	int frameSize = (int)(frameSizeInTime*(double)w.WAVEParams.sampleRate); int frameShiftSize = (int)(frameShiftSizeInTime*(double)w.WAVEParams.sampleRate);
	if (w.WAVEParams.numChannels > 1) { printf("Only use one channel\n"); }
	printf("Frame length : %f s\nFrame shift length : %f s \n", frameSizeInTime, frameShiftSizeInTime);
	Matrix frameStackOfSignal = frameRawSignal(w.DATA.data[1], frameSize, frameShiftSize,0.97,1);
	int numFrames = NumRows(frameStackOfSignal);

	//Calculate zero-crossing rate
	Vector zeroCrossingRateForFrames = CreateVector(numFrames);
	for (int i = 1; i <= VectorSize(zeroCrossingRateForFrames); i++)zeroCrossingRateForFrames[i]=zeroCrossingRate(frameStackOfSignal[i], frameSize);

	//Calculate energy
	Vector energyForFrames = CreateVector(numFrames);
	ZeroVector(energyForFrames);
	for (int i = 1; i <= VectorSize(energyForFrames); i++) {
		for (int j = 1; j <= frameSize; j++)energyForFrames[i] += pow(frameStackOfSignal[i][j], 2.0);
		energyForFrames[i] /= frameSize;
	}


	//set MH,ML and Zs
	IntVec VMH = CreateIntVec(numFrames); IntVec VML = CreateIntVec(numFrames);
	IntVec VZs = CreateIntVec(numFrames); 
	printf("MH = %e \nML = %e \nZs = %e \n", MH, ML, Zs);
	IntVec speechInd = CreateIntVec(numFrames);//Vector to store wheather the frame contains speech
	for (int i = 1; i <= numFrames; i++) {
		VMH[i] = energyForFrames[i] > MH ? 1 : 0;
		VML[i] = energyForFrames[i] > ML ? 1 : 0;
		VZs[i] = zeroCrossingRateForFrames[i] > 3.0*Zs ? 1 : 0;
	}

	int pos = 0;
	CopyIntVec(VMH, speechInd);
	for (int epoch = 1; epoch <= 2; epoch++) {
		//extend speechInd from MH to ML
		pos = 0;
		
		for (int i = 2; i <= numFrames - 1; i++) {
			if (speechInd[i] == 0 && speechInd[i + 1] == 1) {
				pos = i;
				while (pos >= 1 && pos <= numFrames&&VML[pos] == 1) { speechInd[pos] = 1; pos--; }
			}
			if (speechInd[i - 1] == 1 && speechInd[i] == 0) {
				pos = i;
				while (pos >= 1 && pos <= numFrames&&VML[pos] == 1) { speechInd[pos] = 1; pos++; }
			}
		}

		//extend speechInd to Zs
		for (int i = 2; i <= numFrames - 1; i++) {
			if (speechInd[i] == 0 && speechInd[i + 1] == 1) {//01
				pos = i;
				while (pos >= 1 && pos <= numFrames&&VZs[pos] == 1) { speechInd[pos] = 1; pos--; }
			}
			if (speechInd[i - 1] == 1 && speechInd[i] == 0) {//10
				pos = i;
				while (pos >= 1 && pos <= numFrames&&VZs[pos] == 1) { speechInd[pos] = 1; pos++; }
			}
		}
	}

	//if the gap between two speech segments is less than gapLeast,combine the segments into one
	pos = 1;
	printf("Time gap between segment : %f\n", gapLeast);
	for (int i = 2; i <= numFrames - 1; i++) {
		if (speechInd[i - 1] == 1 && speechInd[i] == 0) pos = i;
		if (speechInd[i] == 0 && speechInd[i + 1] == 1&&pos!=0) {
			if ((i - pos) <= (int)(gapLeast / frameShiftSizeInTime)) for (int j = pos; j <= i; j++)speechInd[j] = 1;
			pos = 0;
		}
	}

	//The data to plot
	FILE* fdataOut = fopen("dataToPlot.dat", "w");
	WriteVectorE(fdataOut, zeroCrossingRateForFrames);
	WriteVectorE(fdataOut, energyForFrames);
	WriteIntVec(fdataOut, speechInd);
	fclose(fdataOut);
	

	//print the silent segments
	//pos = 0;
	//printf("the silence segments\n");
	//for (int i = 2; i <= numFrames; i++) {
	//	if (speechInd[i-1]&&!speechInd[i])pos = i;//10
	//	if (speechInd[i]&&pos) {//01
	//		printf("%f --- %f \n", frameShiftSizeInTime*(double)(pos-1), frameShiftSizeInTime*(double)i);
	//		pos = 0;
	//	}
	//}

	//print the speech segments and store these information into a linkList
	pos = 1; activeSeg* head = NULL; activeSeg* tail = NULL; activeSeg* tempP = NULL;
	printf("\nthe voice active segments(s) : \n");
	for (int i = 2; i <= numFrames-1; i++) {
		if (!speechInd[i - 1] && speechInd[i])pos = i;//01
		if (speechInd[i] && (!speechInd[i+1] || i== numFrames - 1) &&pos) {	//10
			tempP = (activeSeg*)malloc(sizeof(activeSeg));
			tempP->startSampleNum = (pos - 1)*frameShiftSize + 1;
			tempP->endSampleNum = (i == numFrames - 1) ? w.WAVEParams.numSamples : i*frameShiftSize;
			tempP->link = NULL;
			if (head == NULL&&tail == NULL)head = tail = tempP;
			else {
				tail->link = tempP; tail = tempP;
			}
			
			printf("%.2f --- %.2f\t", frameShiftSizeInTime*(double)(pos), frameShiftSizeInTime*(double)(i));
			pos = 0;
			
		}
	}


	//save audio files
	char saveFileDes1[300] = { '\0' }; char saveFileDes2[] = { '\\','\0' }; char saveFileDes3[5] = { '\0' }; char saveFileDes4[6] = { '.','w','a','v','\0' };
	FILE* waveForWrite = NULL;
	int saveFileCounter = 0;
	WAVEParams_t paramsForWrite = w.WAVEParams;
	IntMat samplesForWrite = NULL;
	tempP = head;
	printf("\nBegin writing audio\n");
	while (tempP) {
		saveFileCounter++;
		if (argc >= 3) { 
			strcpy(saveFileDes1, argv[2]);
			strcat(saveFileDes1, saveFileDes2);
		}
		else saveFileDes1[0] = '\0';

		_itoa(saveFileCounter, saveFileDes3,10);
		strcat(saveFileDes1, saveFileDes3);
		strcat(saveFileDes1, saveFileDes4);

		waveForWrite = fopen(saveFileDes1, "wb");
		paramsForWrite.numSamples = tempP->endSampleNum - tempP->startSampleNum + 1;
		samplesForWrite = CreateIntMat(paramsForWrite.numChannels, paramsForWrite.numSamples);
		for (int i = tempP->startSampleNum; i <= tempP->endSampleNum; i++)for (int j = 1; j <= paramsForWrite.numChannels; j++)samplesForWrite[j][i - tempP->startSampleNum + 1] = w.DATA.data[j][i];
		writeWaveFile(waveForWrite, paramsForWrite, samplesForWrite);
		fclose(waveForWrite);
		printf("%s\t", saveFileDes1);

		FreeIntMat(samplesForWrite);
		tempP = tempP->link;
	}
	
	

	//free linkList
	do {
		tempP = head->link;
		free(head);
		head = tempP;
	} while (head);
	head = tail = tempP = NULL;

	/*
	int frameSize2 = (int)(genderDectectionWindow*(double)w.WAVEParams.sampleRate); int frameShiftSize2 = (int)(genderDectectionWindow / 2 * (double)w.WAVEParams.sampleRate);
	Matrix frameStackOfSignal2 = frameRawSignal(w.DATA.data[1], frameSize2, frameShiftSize2);
	int numFrames2 = NumRows(frameStackOfSignal2);
	*/
	

	/*
	int fftSize = (int)pow(2.0,ceil(log2(frameSize)));
	Matrix frameStackOfSignalForFFT = CreateMatrix(numFrames, 2* fftSize);
	ZeroMatrix(frameStackOfSignalForFFT);
	for (int i = 1; i <= numFrames; i++)for (int j = 1; j <= frameSize; j++)frameStackOfSignalForFFT[i][2 * j - 1] = frameStackOfSignal[i][j];
//	XFFT xf; VectorToXFFT(&xf, frameStackOfSignalForFFT[1]); ShowXFFT(xf);

	for (int i = 1; i <= numFrames; i++)FFT(frameStackOfSignalForFFT[i], 0);
	Vector xfPow = CreateVector(fftSize/2); IntVec maxIndInFFT = CreateIntVec(numFrames);
	for (int i = 1; i <= numFrames; i++) {
		for (int j = 1; j <= fftSize/2; j++)xfPow[j] = pow(frameStackOfSignalForFFT[i][2 * j - 1], 2.0) + pow(frameStackOfSignalForFFT[i][2 * j], 2.0);
		maxIndInFFT[i]=FindMaxIndex(xfPow);
	}
	WriteIntVec(f, maxIndInFFT); fclose(f);

//	XFFT xf; VectorToXFFT(&xf, frameStackOfSignalForFFT[1]); ShowXFFT(xf);

*/

	free_WAVE(&w); FreeMatrix(frameStackOfSignal); FreeVector(zeroCrossingRateForFrames); FreeVector(energyForFrames);
	FreeIntVec(VMH); FreeIntVec(VML); FreeIntVec(VZs); FreeIntVec(speechInd);

	system("pause");
	return 0;
}