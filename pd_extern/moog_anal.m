clear all;
x = wavread("moog_r1.wav");
%plot(x);
anal_per = 9182;
cnt = 1;
sz = size(x);
;

% window function
for n=1:anal_per
  win(n) = .5 * (1 - cos( (2*pi*(n-1))/(anal_per-1)));
end
win = win';

n=1;
while cnt<length(x)-2*anal_per
  seg = x(cnt:cnt+anal_per-1) .* win;
  mags(n,:) = 20 *log10(sqrt(abs(fft(seg)/anal_per)));
  cnt = cnt+anal_per;
  n = n+1;
end

scale = linspace(0,44100,44100);
figure 1;
hold all;
for m=1:n-1
  plot(mags(m,:))
end

hold off;
