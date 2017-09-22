#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include"fft.h"
#include"hmath.h"
#include"sigProcess.h"
#include"WAVE.h"

typedef struct {
	int startSampleNum;
	int endSampleNum;
	struct activeSeg* link;
}activeSeg;

int main(int argc, char** argv) {
	printf("%s\n", argv[0]);
	printf("This program is for VAD. Input the audio to be detected and the dir to store the transformed audio.\n ");
	//open WAVE and read
	WAVE_t w = initWAVE_t();
	FILE* fWAVE = NULL;
	if (argc >= 2) { fWAVE = fopen(argv[1], "rb"); printf("WAVE information:\nname: %s\n", argv[1]);}
	else { fWAVE = fopen("PHONE_001.wav", "rb"); printf("WAVE information:\nname: PHONE_001.wav\n");}
	loadWAVEFile(&w, fWAVE);
	print_WAVE(w);

	//Frame Signal
	double frameSizeInTime = 0.025; double frameShiftSizeInTime = 0.010;
	int frameSize = (int)(frameSizeInTime*(double)w.WAVEParams.sampleRate); int frameShiftSize = (int)(frameShiftSizeInTime*(double)w.WAVEParams.sampleRate);
	if (w.WAVEParams.numChannels > 1) { printf("Only use one channel\n"); }
	Matrix frameStackOfSignal = frameRawSignal(w.DATA.data[1], frameSize, frameShiftSize);
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

/*	FILE* f = fopen("temp.dat", "w"); 
	WriteVectorE(f, zeroCrossingRateForFrames); 
	WriteVectorE(f, energyForFrames);
	fclose(f);
	*/

	//set MH,ML and Zs
	double MH =(double)1e5, ML = (double)1e4, Zs = 0.06;
	IntVec VMH = CreateIntVec(numFrames); IntVec VML = CreateIntVec(numFrames);
	IntVec VZs = CreateIntVec(numFrames); 
	IntVec speechInd = CreateIntVec(numFrames);//Vector to store wheather the frame contains speech
	for (int i = 1; i <= numFrames; i++) {
		VMH[i] = energyForFrames[i] > MH ? 1 : 0;
		VML[i] = energyForFrames[i] > ML ? 1 : 0;
		VZs[i] = zeroCrossingRateForFrames[i] > 3.0*Zs ? 1 : 0;
	}

	//extend speechInd from MH to ML
	int pos = 0;
	CopyIntVec(VMH, speechInd);
	for (int i = 2; i <= numFrames-1; i++) {
		if (speechInd[i] == 0 && speechInd[i + 1] == 1) {
			pos = i;
			while (pos>=1&&pos<=numFrames&&VML[pos] == 1) { speechInd[pos] = 1; pos--; }
		}
		if (speechInd[i-1] == 1 && speechInd[i] == 0) {
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

	//if the gap between two speech segments is less than gapLeast,combine the segments into one
	pos = 1; double gapLeast = 1.0;
	for (int i = 2; i <= numFrames - 1; i++) {
		if (speechInd[i - 1] == 1 && speechInd[i] == 0) pos = i;
		if (speechInd[i] == 0 && speechInd[i + 1] == 1&&pos!=0) {
			if ((i - pos) <= (int)(gapLeast / frameShiftSizeInTime)) for (int j = pos; j <= i; j++)speechInd[j] = 1;
			pos = 0;
		}
	}

	//print the silent segments
	pos = 0;
	printf("the silence segments\n");
	for (int i = 2; i <= numFrames; i++) {
		if (speechInd[i-1]&&!speechInd[i])pos = i;//10
		if (speechInd[i]&&pos) {//01
			printf("%f --- %f \n", frameShiftSizeInTime*(double)(pos-1), frameShiftSizeInTime*(double)i);
			pos = 0;
		}
	}

	//print the speech segments and store these information into a linkList
	pos = 1; activeSeg* head = NULL; activeSeg* tail = NULL; activeSeg* tempP = NULL;
	printf("the voice active segments\n");
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
			
			printf("%f --- %f \n", frameShiftSizeInTime*(double)(pos), frameShiftSizeInTime*(double)(i));
			pos = 0;
			
		}
	}

	
	tempP = head;
	do {
		//pass
		tempP = tempP->link;
	} while (tempP);

	//free linkList
	do {
		tempP = head->link;
		free(head);
		head = tempP;
	} while (head);
	head = tail = tempP = NULL;
	

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