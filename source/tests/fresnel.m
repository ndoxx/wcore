clear all;
close all;

F0 = 0.04;
vh = [0:0.01:1];

% Fresnel-Schlick approximation
Fs  = @(x) F0 + (1-F0)*(1-x).^5;
% Gaussian spherical approximation
Fgs = @(x) F0 + (1-F0)*2.^((-5.55473.*x - 6.98316).*x);

plot(vh, [Fs(vh); Fgs(vh)]);