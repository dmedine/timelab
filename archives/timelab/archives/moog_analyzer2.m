x = load("../moog_printout1");

h = hann(1024);

num_frames = sz/1024;
bin_val = 48000/512;
%we know that the resonance  = 4 at frame 400
m = zeros(1024, num_frames);
fftm = zeros(1024, num_frames);
magsm = zeros(1024, num_frames);


for(n=1:num_frames)
  start = (n-1)*1024 + 1;
  fin = n*1024;
  m(:,n) = x(start : fin);
  
end%for

for(n=1:num_frames)
    
  fftm(:,n) = fft( h .* m(:,n))/1024;
  magspec(:,n) = sqrt(abs(fftm(:,n)));
  maxpow = max(magsm(:,n));

end%for



