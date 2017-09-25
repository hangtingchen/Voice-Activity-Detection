clc;clear;
m=load('C:\Users\Administrator\Documents\Visual Studio 2015\Projects\VAD\VAD\dataToPlot.dat');
[a,fs]=audioread('C:\Users\Administrator\Documents\Visual Studio 2015\Projects\VAD\VAD\PHONE_001.wav','native');
t=20;frameShift=0.01;
a0=a(1:fs*t);time0=(1:fs*t)/fs;
zerocros=m(1,1:t/frameShift);energy0=m(2,1:t/frameShift);
timeFrame0=(1:t/frameShift)*frameShift;
bar0=find(m(3,1:t/frameShift));timeFrameBar0=timeFrame0(bar0);

figure(1);

subplot(2,1,1)
plot(time0,a0,'b');xlabel('Time');ylabel('WAVE');title('Silence > 0.1s');
axis([0 t -max(abs(a0)) max(abs(a0))]);
hold on;
bar(timeFrameBar0,double(max(abs(a0)))*ones(size(bar0)),1.0,'g','EdgeColor','none');alpha(0.3);
bar(timeFrameBar0,-double(max(abs(a0)))*ones(size(bar0)),1.0,'g','EdgeColor','none');alpha(0.3);


subplot(2,1,2);
[AX,H1,H2] = plotyy(timeFrame0,energy0,timeFrame0,zerocros,'plot');
set(AX(1),'XColor','k','YColor','b');
set(AX(2),'XColor','k','YColor','r');
HH1=get(AX(1),'Ylabel');
set(HH1,'String','Short-time average energy');
set(HH1,'color','k');
HH2=get(AX(2),'Ylabel');
set(HH2,'String','Zero-crossing rate');
set(HH2,'color','r');
set(H1,'LineStyle','-');
set(H1,'color','b');
set(H2,'LineStyle','-');
set(H2,'color','r');
%legend([H1,H2],{'y1 = 200*exp(-0.05*x).*sin(x)';'y2 = 0.8*exp(-0.5*x).*sin(10*x)'});
xlabel('Time');
%title('First 20s in audio PHONE_001.wav');