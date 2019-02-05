clear all;
close all;

edge_factor = @(x,xmax,dx) min(1, x ./ dx) .* min(1, (xmax-x) ./ dx);

[X,Y] = meshgrid(0:1:31,0:1:31);
Z = edge_factor(X,31,5) .* edge_factor(Y,31,5);

% plot(edge_factor(0:33,33,2));
surf(X,Y,Z);