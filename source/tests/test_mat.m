clear all;
close all;

T = [1.0 0.0 0.0 11.0;
  0.0 1.0 0.0 -12.0;
  0.0 0.0 1.0 13.0;
  0.0 0.0 0.0 1.0]
  
a = pi/4;

Rx = [1.0 0.0 0.0 0.0;
    0.0 cos(a) -sin(a) 0.0;
    0.0 sin(a) cos(a) 0.0;
    0.0 0.0 0.0 1.0]
Ry = [cos(a) 0.0 -sin(a) 0.0;
    0.0 1.0 0.0 0.0;
    sin(a) 0.0 cos(a) 0.0;
    0.0 0.0 0.0 1.0]
Rz = [cos(a) -sin(a) 0.0 0.0;
    sin(a) cos(a) 0.0 0.0;
    0.0 0.0 1.0 0.0;
    0.0 0.0 0.0 1.0]

M4 = Rx * Ry * Rz * T
inv(M4)
M4(4) = 0.1;
M4
inv(M4)

M = [1.0 2.0 3.0;
     3.0 4.0 5.0;
     8.0 9.0 7.0]
 
inv(M)

%% quats

q = [1 0 1 0];
r = [1 0.5 0.5 0.75];
qr = quatmultiply(q, r)
rq = quatmultiply(r, q)

q2 = [1.5 0.05 1.2 0.1];
r2 = [1.5 0.8 0.1 2.75];

qr2 = quatmultiply(q2, r2)
rq2 = quatmultiply(r2, q2)

q1 = [1.0 2.0 0.5 -0.75];
q1i = quatinv(q1)

q = [1 0 1 0];
r = [1 0.5 0.5 0.75];
dqr = quatdivide(q, r)
drq = quatdivide(r, q)

q = [1 0 1 0];
r = [1 0.5 0.3 0.1];
v = [1 1 1];
quatrotate(q,v)
quatrotate(r,v)

%%
Mq = quat2rotm(q)
Mr = quat2rotm(r)

M = [0.8 0.1 0.7;
     0.5 0.48 -0.94;
     -0.5 1.06 0.32];
M*v'

%%
eulx = [pi/4 0 0];
euly = [0 pi/4 0];
eulz = [0 0 pi/4];
qx = eul2quat(eulx)
qy = eul2quat(euly)
qz = eul2quat(eulz)

%%
q1 = [0.7071 0.7071 0 0];
q2 = [0.7071 0 0.7071 0];
q3 = [2.1 0.75 -1.25 3.21];
% quatmultiply(q1,q2)
% quatmultiply(q2,q1)

% quatdivide(q1,q2)
% quatdivide(q2,q1)
v = [1 1 1];
v2 = [-0.5 0.2 1.8];
quatrotate(q1,v)
quatrotate(q2,v)
quatrotate(q3,v)
quatrotate(q3,v2)

%%
eul = [30.0 0.0 0.0];
c = cos(eul/2);
s = sin(eul/2);
q = [c(:,1).*c(:,2).*c(:,3)+s(:,1).*s(:,2).*s(:,3), ...
    c(:,1).*c(:,2).*s(:,3)-s(:,1).*s(:,2).*c(:,3), ...
    c(:,1).*s(:,2).*c(:,3)+s(:,1).*c(:,2).*s(:,3), ...
    s(:,1).*c(:,2).*c(:,3)-c(:,1).*s(:,2).*s(:,3)]

%%
R1 = quat2rotm([0.7071 0.7071 0 0]);
R2 = quat2rotm([0.7071 0 0.7071 0]);
R1 = [R1; 0 0 0];
R1 = [R1, [0 0 0 1]'];
R2 = [R2; 0 0 0];
R2 = [R2, [0 0 0 1]'];

T = [1, 0, 0, 15.4;
     0, 1, 0, -128.2;
     0, 0, 1, 114.7;
     0, 0, 0, 1.0];
M = R2*R1*T
inv(M)

%% Euler angles to rotation matrix

eulx = [0 0 pi/3];
euly = [0 pi/3 0];
eulz = [pi/3 0 0];
rotmx = eul2rotm(eulx)
rotmy = eul2rotm(euly)
rotmz = eul2rotm(eulz)


%% 

M = rand(4,4)
d = det(M)

%% Experiment: ortho <--> persp matrix interpolation

aspect = 4.0/3.0;
l = -aspect/2;
r = aspect/2;
b = -0.5;
t = 0.5;
n = 0.5;
f = 100.0;


Mortho = [1.0/r 0 0 0;
          0 1.0/t 0 0;
          0 0 -2.0/(f-n) -(f+n)/(f-n);
          0 0 0 1]

Mpersp = [n/r 0 0 0;
          0 n/t 0 0;
          0 0 -(f+n)/(f-n) -2*f*n/(f-n);
          0 0 -1 0]

V = [1 1 1 1]';

Mortho_inv = inv(Mortho);
Mpersp_inv = inv(Mpersp);

Mop = Mpersp*Mortho_inv;
Mpo = Mortho*Mpersp_inv;

vo = Mortho*V;
% vp = Mpersp*V;
vop = Mop*vo;
% vpo = Mpo*vp;
vop = vop/vop(4);
% vpo = vpo/vpo(4)
vopexpect = Mpersp*V;
% vpoexpect = Mortho*V;
vopexpect = vopexpect/vopexpect(4);
% vpoexpect = vpoexpect/vpoexpect(4)

x = -1:0.2:1;
y = -1:0.2:1;
z = -1:0.5:1;

z = z + 2;

X = [];
Y = [];
Z = [];
vo = [];
vp = [];
vi = [];

alpha = 1.0;

for ii=[1:size(x,2)]
    for jj=[1:size(y,2)]
        for kk=[1:size(z,2)]
            X = [X x(ii)];
            Y = [Y y(jj)];
            Z = [Z z(kk)];
            V = [x(ii) y(jj) z(kk)];
            newvo = (Mortho*[V 1]')';
            newvp = (Mpersp*[V 1]')';
            newvo = newvo/newvo(4);
            newvp = newvp/newvp(4);
            newvi = alpha*newvp + (1-alpha)*newvo;
            vo = [vo; newvo];
            vp = [vp; newvp];
            vi = [vi; newvi];
        end
    end
end

hold on;
scatter3(vo(:,1),vo(:,2),vo(:,3),'r+');
scatter3(vi(:,1),vi(:,2),vi(:,3),'b+');
% scatter3(vp(:,1),vp(:,2),vp(:,3),'b+');
hold off;
axis tight;


%% Proj matrix interp explosivity test

dop = []; % det of ortho->persp interpolated matrix
dpo = []; % det of persp->ortho interpolated matrix

dsop = []; % det of Sigma matrix in SVD(Mop)
dspo = []; % det of Sigma matrix in SVD(Mpo)

evs = [];
evpo = [];

for alpha=[0:0.01:1]
    Mop = (1-alpha)*Mortho + alpha*Mpersp;
    Mpo = (1-alpha)*Mpersp + alpha*Mortho;
    
    dop = [dop; det(Mop)];
    dpo = [dpo; det(Mpo)];
    
    [~,S,~] = svd(Mop);
    dsop = [dsop; det(S)];
    [~,S,~] = svd(Mpo);
    dspo = [dspo; det(S)];
    
    [~,D] = eig(Mop);
    ev = diag(D)';
    evs = [evs; ev];
    
    [~,D] = eig(Mpo);
    ev = diag(D)';
    evpo = [evpo; ev];
end

figure(1);
subplot(2,1,1); plot([1./dop 1./dpo]); legend('1/det(M_{op})','1/det(M_{po})');
subplot(2,1,2); plot([1./dsop 1./dspo]); legend('1/det(\Sigma_{op})','1/det(\Sigma_{po})');

% => So the determinant of sigma matrix in SVD of interp matrix is the
% direct measure of what is "wrong" in the transformation.

figure(2);
subplot(2,1,1); plot(evs); legend('\lambda_1','\lambda_2','\lambda_3','\lambda_4');
subplot(2,1,2); plot(evpo); legend('\lambda_1','\lambda_2','\lambda_3','\lambda_4');

% => Sens ortho -> changement brutal des lambdas dès le premier pas.
% => Sens persp -> changement brutal des lambdas à la fin, juste avant ortho.
% => Singularité proche ortho => Chercher quelle dim est la plus
% "susceptible".

%% Non linear interp
clear all;

t = 0:0.01:1;
a = -1:0.1:1;
[tt,aa] = meshgrid(t,a);

P = @(t,alpha) alpha.*(t.^2) + (1-alpha).*t;

% curves = P(tt,aa)';
% plot(t,curves);
% xlim([0 1]);

% => P is our quadratic interpolant for -1<alpha<1

aspect = 4.0/3.0;
l = -aspect/2;
r = aspect/2;
b = -0.5;
t = 0.5;
n = 0.5;
f = 100.0;


Mortho = [1.0/r 0 0 0;
          0 1.0/t 0 0;
          0 0 -2.0/(f-n) -(f+n)/(f-n);
          0 0 0 1];

Mpersp = [n/r 0 0 0;
          0 n/t 0 0;
          0 0 -(f+n)/(f-n) -2*f*n/(f-n);
          0 0 -1 0];

Dets = [];
l1 = [];
l2 = [];
l3 = [];
l4 = [];
t=0:0.01:1;
for kk=1:100
    dets = [];
    evs = [];
    alphas = 2*rand(1,6)-1;
    for tt=t
        coeffs = zeros(1,6);
        for ii=1:6
            coeffs(ii) = alphas(ii)*tt^2 + (1-alphas(ii))*tt;
        end
        xScale = (1-coeffs(1))*Mortho(1) + coeffs(1)*Mpersp(1);
        yScale = (1-coeffs(2))*Mortho(6) + coeffs(2)*Mpersp(6);
        zScale = (1-coeffs(3))*Mortho(11) + coeffs(3)*Mpersp(11);
        wz =     (1-coeffs(4))*Mortho(12) + coeffs(4)*Mpersp(12);
        zw =     (1-coeffs(5))*Mortho(15) + coeffs(5)*Mpersp(15);
        ww =     (1-coeffs(6))*Mortho(16) + coeffs(6)*Mpersp(16);

        Minterp = [xScale 0 0 0;
                   0 yScale 0 0;
                   0 0 zScale zw;
                   0 0 wz ww];
        dets = [dets det(Minterp)];
        
        [~,D] = eig(Minterp);
        ev = diag(D)';
        evs = [evs; ev];
    end
    Dets = [Dets; dets];
    l1 = [l1 evs(:,1)];
    l2 = [l2 evs(:,2)];
    l3 = [l3 evs(:,3)];
    l4 = [l4 evs(:,4)];
end

%% plot
figure(1);
subplot(3,2,1); plot(t, l1); title('\lambda_1(t)'); xlabel('t'); ylabel('\lambda_1');
subplot(3,2,2); plot(t, l2); title('\lambda_2(t)'); xlabel('t'); ylabel('\lambda_2');
subplot(3,2,3); plot(t, l3); title('\lambda_3(t)'); xlabel('t'); ylabel('\lambda_3');
subplot(3,2,4); plot(t, l4); title('\lambda_4(t)'); xlabel('t'); ylabel('\lambda_4');
subplot(3,2,[5 6]); plot(t, Dets'); title('det(M_\alpha)(t)'); xlabel('t'); ylabel('det(M_\alpha)');
      
      
% => There's no escaping the abrupt behavior near ortho by allowing for
% independent quadratic interp of matrices components.
% => Maybe "scale continuity pbm"
      
      