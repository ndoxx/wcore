clear all;
close all;

%clamp((1.0 - pow(distance/radius, 2.0)),0.0,1.0)/(1.0 + distance*distance);

f = @(r,R) (1.0 - (r./R).^2) ./ (1.0 + r.^2);


R=10.0;
I=R;
r=0:0.01:R;
plot(r,I*f(r,R));