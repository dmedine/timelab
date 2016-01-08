clear all;
x1 = wavread("moog_r1.wav");
x2 = wavread("moog_r2.wav");
x3 = wavread("moog_r3.wav");
x4 = wavread("moog_r4.wav");

function [mags] = get_mags(x)
  

  anal_per = 9182;
  cnt = 1;
  sz = size(x);
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


end%function

mags1 = get_mags(x1);
mags2 = get_mags(x2);
mags3 = get_mags(x3);
mags4 = get_mags(x4);

scale = linspace(0,44100,44100);
figure 1;
hold all;
sz = size(mags1);
for m=1:sz(1)-1
  plot(mags4(m,:))
end

hold off;
