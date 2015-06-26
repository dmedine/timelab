x = load("../moog_printout");
g = load("../moog_printout_ctls");
len = 512;
half_len = len/2;

h = hann(len);

num_frames = 60;%size(x)(1)/len;
bin_val = 48000/len;
%we know that the resonance  = 4 at frame 400
m = zeros(len, num_frames);
fftm = zeros(len, num_frames);
magsm = zeros(len, num_frames);
maxs = zeros(1,num_frames);
peaks = zeros(1,num_frames);
tar_peaks = num_frames;
plot_scale_peaks = linspace(100:5000, num_frames);
	
for(n=1:num_frames)
  start = (n-1)*len + 1;
  fin = n*len;
  m(:,n) = x(start : fin);
  
end%for

fftm = m;
	
for(n=1:num_frames)
  fftm(:,n) = fft(h .* fftm(:,n))/len;
  magsm(:,n) = abs(fftm(:,n));
  maxs(n) = max(magsm(1:half_len, n));
end%for

for(n=1:num_frames)
   for(j=1:half_len)
     tmp = magsm(j, n);
      if(tmp == maxs(n))
	peaks(n) = j;
      end%if

   end%for
end%for
peaks *=bin_val;
plot(peaks, '1x');
hold;
%plot(g(1:60,3), 'o');
title("cutoff frequency control value and measured frequency")
xlabel("analysis window")
ylabel("frequency in Hz");


%hold off;



%end%function
