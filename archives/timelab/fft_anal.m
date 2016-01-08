clear;
SR = 48000;

x = load("sho_m5_out_mass");
y = load("sho_m5_out_sprng");

len = size(x)(1) - 10000;
hann = zeros(len,1);
for(n = 1:len)

  hann(n) = .5 * (1 - cos( (2 * pi * (n-1)) /len));

end%for

%plot(hann);

for(n=1:size(x)(2))

   xwin(:,n) = x(10001:size(x)(1),n).*hann;
   ywin(:,n) = y(10001:size(x)(1),n).*hann;
   
end%for

str = zeros(size(x)(2),size(x)(1));

for(n=1:size(x)(1))

  str(:,n) = x(n,:)';

end%for

magsx = 20 * log10(abs(fft(xwin)/len));
magsy = 20*log10(abs(fft(ywin)/len));

phsx = angle(fft(xwin)/len);
phsy = angle(fft(ywin)/len);

scale = 0 : SR/len : SR - 1/len;

%plot(scale, magsx);

m = 1/100;%unit mass
sc = 1/100;%tension
npts = 10;
mu = size(x)(2)/48000;
c = sqrt(sc/m);
length = npts*mu
fund = c/(2*length)
