l = linspace(-10,10,1000);
m0 = -1;
m1 = 1;
bp0 = 1;
bp1 = -1;
fact = .5;
f = m1 * l+ fact*(m0-m1) .* (abs(l+bp0)  - abs(l-bp1));
plot(l,f)
