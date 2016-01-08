x = load("../moog_printout3");
sz = size(x)(1);
len=1024;
h = hann(len);

num_frames = sz/len;
%we know that the resonance  = 4 at frame 400
m = zeros(len, num_frames);
fftm = zeros(len, num_frames);
magsm = zeros(len, num_frames);


for(n=1:num_frames)
  start = (n-1)*len + 1;
  fin = n*len;
  m(:,n) = x(start : fin);
  
end%for

for(n=1:num_frames)
    
  fftm(:,n) = fft( h .* m(:,n))/len;
  magspec(:,n) = sqrt(abs(fftm(:,n)));
  maxpow = max(magsm(:,n));

end%for



