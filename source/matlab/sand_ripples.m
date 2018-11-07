function [Hnew,M] = ripples2(H,hopX,windX,hopY,windY,grain,gravity,critAng,numsteps)

[rows cols] = size(H);
Heven = H;
Hodd = H;
for currstep = 1:numsteps
    
    % Hodd blowing to Heven
    Heven = Hodd;
    for x = 1:rows
        
        for y = 1:cols
            
            
          %%%%%%%%%%%%%%%%%%%%%%%%%%%%%     
          % here we define the coordinates for all surrounding neighbors of a
          % point
          
            if (x==1)
                xUp=rows;
            else
                xUp = x-1;
            end
                                
            if (x==rows)        
                xDown=1;
            else
                xDown = x+1;    
            end
            
            if (y==1)
                yLeft=cols;
            else
                yLeft = y-1;
            end
if (y==cols)
                yRt=1;
            else
                yRt = y+1;
            end
            
            h = Hodd(x,y);
            hR = Hodd(x,yRt); hRD = Hodd(xDown,yRt); hRU = Hodd(xUp,yRt);
            hL = Hodd(x,yLeft); hLD = Hodd(xDown,yLeft); hLU = Hodd(xUp,yLeft);
            hD = Hodd(xDown,y); hU = Hodd(xUp,y);
            
          
          %%%%%%%%%%%%%%%%%%
          % SALTATION
          
          % we define the gradients
          
            delHx = Hodd(xDown,y)-Hodd(x,y);
            delHy = Hodd(x,yRt) - Hodd(x,y);
            delH  = sign(delHx)*sqrt(delHx^2 + delHy^2);
           
          % the hop length depends on the height and slope
            hopLengthX = (hopX + windX*Hodd(x,y))*(1-tanh(delHx));
            hopLengthY = (hopY + windY*Hodd(x,y))*(1-tanh(delHy));             
            
          % the amount of grains transported depends on the slope, delH
            grainAmt = (grain)*(1+tanh(delH));
                            
            % this is where the grains blow
            blowToX = round(x + hopLengthX); 
            blowToY = round(y + hopLengthY);
            
            if (blowToX > rows)
                blowToX = blowToX - rows;
            end
            
            if (blowToY > cols)
                blowToY = blowToY - rows;
            end
                        
            Heven(x,y) = Heven(x,y) - grainAmt;
            Heven(blowToX,blowToY) = Heven(blowToX,blowToY) + grainAmt; 
            
            %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
            % CREEP due to gravity
            % here we're taking a weighted sum of the 4 immediate neighbors
            
            FirstNbrSum = 1/6*( hU + hD + hL + hR );
            
            % we take a less-weighted sum of the four "corner neighbors"
            SecondNbrSum = 1/12*( hLU + hRU + hLD + hRD);
                                
            
            Heven(x,y) = Heven(x,y) + gravity*(FirstNbrSum+SecondNbrSum - h);
            
            
            %%%%%%%%%%%%%%%%%%%%%%%%%%%
            %  AVALANCHE if the slope gets too big
           
            sandIndex = [];
            b = [];
            
            if ((h-hR)/1 > tan(critAng))
               sandIndex = [sandIndex;1];
               b = [b;tan(critAng)+hR-h];
            end
           
            if (h-hRD)/(sqrt(2)) > tan(critAng)
               sandIndex = [sandIndex;2];
               b = [b;sqrt(2)*tan(critAng)+hRD-h];               
            end
           
            if (h-hD)/1 > tan(critAng)
               sandIndex = [sandIndex;3];
               b = [b;tan(critAng)+hD-h];
            end
           
            if (h-hLD)/(sqrt(2)) > tan(critAng)
               sandIndex = [sandIndex;4];
               b = [b;sqrt(2)*tan(critAng)+hLD-h];    
            end

            n = length(b);
            A = -ones(n);
            
            for i = 1:n
                A(i,i)= -2;
            end
            
            sandShift = A\b;
            
            for j = 1:n
                if sandIndex(j) == 1
                    Heven(x,yRt)=hR + sandShift(j);
                elseif sandIndex(j) == 2
                    Heven(xDown,yRt)=hRD + sandShift(j);
                elseif sandIndex(j) == 3
                    Heven(xDown,y)=hD + sandShift(j);
                elseif sandIndex(j) == 4
                    Heven(xDown,yLeft)=hLD + sandShift(j);
                end
            end
            
            Heven(x,y)-sum(sandShift);
            
            end
        end
        Hodd = Heven;
    
       meshz(Hodd) 
       axis([0 100 0 100 0 50])
       pause(.01)
       
       if mod(currstep,3)==0
           M(currstep/3)=getframe;
       end  
end
Hnew = Hodd;