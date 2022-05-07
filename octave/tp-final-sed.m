pkg load signal

clc
clear all

data = csvread("A1651939057654.csv");

x = data(2:end, 1)';
y = data(2:end, 2)';

% Inciso 4.1
 
% Gráfica de los datos.
figure;
plot(x, y);
title ("Señal time vs datos");

% Gráfica del histograma con 10 bines.
figure; 
hist(y,10);
title ("Histograma con 10 bins");

% Gráfica del histograma con 50 bines
figure; 
hist(y,50);
title ("Histograma con 50 bins");

% Estimación de la media y la varianza.
mean = mean(y);
variance = var(y);

% Inciso 4.2

[rows, columns] = size(y);
y1 = zeros(1,columns);

for i = 1:columns
  y1(i) = y(i) - mean; 
endfor

figure;
specgram(y1, 1024, 100, 300, 280);
title ("Espectograma de la señal");
colormap('jet');

% Inciso 4.3

figure;
random = 3135*randn([1, 6000]);
specgram(random, 1024, 100, 300, 280);
title ("Espectograma de la señal aleatoria");
