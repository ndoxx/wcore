clear all;
close all;

p = [0 1 0; 1 0.2 0.5];
m = [0 0 0; 0 0 0];

x1 = 0;
x2 = 2;
deltax = x2-x1;

c = [ p(1,:);
      deltax*m(1,:);
      3*(p(2,:)-p(1,:)) - deltax*(2*m(1,:)+m(2,:));
     -2*(p(2,:)-p(1,:)) + deltax*(m(1,:)+m(2,:)) ];
   
q = @(t) c(1,:) + c(2,:)*t + c(3,:)*t^2 + c(4,:)*t^3;

val = [];
for xx=x1:deltax/100:x2
    tt = (xx-x1)/deltax;
    val = [val; q(tt)];
end

xx=x1:deltax/100:x2;
h=plot(xx,val);
set(h, {'color'}, {[1 0 0]; [0 1 0]; [0 0 1]});