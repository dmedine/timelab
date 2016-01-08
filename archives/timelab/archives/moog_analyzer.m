x = load("../moog_printout");
g = load("../moog_printout_ctls");
h = hann(1024);

num_frames = size(x)(1)/1024;
bin_val = 48000/512;
%we know that the resonance  = 4 at frame 400
m = zeros(1024, num_frames);
fftm = zeros(1024, num_frames);
magsm = zeros(1024, num_frames);
maxs = zeros(1,num_frames);
peaks = zeros(1,num_frames);
tar_peaks = num_frames;
plot_scale_peaks = linspace(100:5000, num_frames);
	
for(n=1:num_frames)
  start = (n-1)*1024 + 1;
  fin = n*1024;
  m(:,n) = x(start : fin);
  
end%for

fftm = m;
	
for(n=1:num_frames)
  fftm(:,n) = fft(h .* fftm(:,n))/1024;
  magsm(:,n) = abs(fftm(:,n));
  maxs(n) = max(magsm(1:512, n));
end%for

for(n=1:num_frames)
   for(j=1:512)
     tmp = magsm(j, n);
      if(tmp == maxs(n))
	peaks(n) = j;
      end%if

   end%for
end%for
peaks *=bin_val;
plot(peaks, '1x');
hold;
plot(g(:,3), 'o');
title("cutoff frequency control value and measured frequency")
xlabel("windowsize/sample-rate seconds")
ylabel("frequency in Hz");


%hold off;



%end%function
