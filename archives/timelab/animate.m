function  animate(matrix, range)
%assumes a matrix where each
%row is a slice of time data

sz = size(matrix);
figure(1,"visible", "off");
for(n=1:sz(2))
  plot(matrix(:,n));
  axis([1,sz(1),-1*range,range], "square");
  fname = sprintf('output%05d.png', n);
  print(fname);
end%function
figure(1,"visible", "on");
