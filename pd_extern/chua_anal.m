clear all;
x = wavread("chua_x.wav");
y = wavread("chua_y.wav");
z = wavread("chua_z.wav");

plot3(x, y, z);
