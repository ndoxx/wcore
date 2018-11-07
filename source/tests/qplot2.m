clear all;
close all;

M = dlmread('data/data.txt');
plot(M(:,1),M(:,2));