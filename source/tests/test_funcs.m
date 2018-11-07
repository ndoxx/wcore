clear all;
close all;

xx = 0:0.01:5;

sigmoid0 = @(x) (1+tanh(20*(x-1)))/2;
sigmoid1 = @(x) 1./(1+exp(-20*(x-1)));


figure(1);
plot(xx, sigmoid0(xx));

figure(2);
plot(xx, sigmoid1(xx));