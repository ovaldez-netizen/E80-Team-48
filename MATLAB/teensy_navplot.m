%% Accel Demo
% This file simulates a 1-D acceleration measured by an accelerometer with
% noise. It cacluates the true acceleration, velocity and position, and
% then adds gaussian white noise to the true acceleration to generate the
% simulated measured acceleration. It then integrates the measured
% acceleration once to get calculated velocity, and then a second time to
% get calculated position. It calculates the error bounds for the position
% and velocity based on the standard deviation of the sensor and the
% specified confidence level.
dt = 0.1; % The sampling rate
t = 0:dt:25.5; % The time array

t = t';
a = accelX / 998;
ay = accelY / 998;

sigma = std(accelX); % The standard deviation of the noise in the accel.

confLev = 0.95; % The confidence level for bounds
preie = sqrt(2)*erfinv(confLev)*sigma*sqrt(dt); % the prefix to the sqrt(t)
preiie = 2/3*preie; % The prefix to t^3/2a = 1 + sin( pi*t - pi/2);
plusie=preie*t.^0.5; % The positive noise bound for one integration
plusiie = preiie*t.^1.5; % The positive noise bound for double integration

v = cumtrapz(t,a); % Integrate the true acceleration to get the true velocity
r = cumtrapz(t,v); % Integrate the true velocity to get the true position.

sigmaY = std(accelY);
preieY = sqrt(2)*erfinv(confLev)*sigmaY*sqrt(dt);  
preiieY = 2/3*preieY; % The prefix to t^3/2a = 1 + sin( pi*t - pi/2);
plusieY=preieY*t.^0.5; % The positive noise bound for one integration
plusiieY = preiieY*t.^1.5; % The positive noise bound for double integration

vY = cumtrapz(t,ay); % Integrate the true acceleration to get the true velocity
rY = cumtrapz(t,vY); % Integrate the true velocity to get the true position.

rnp = r + plusiie; % Position plus confidence bound
rnm = r - plusiie; % Position minus confidence bound

rnpY = rY + plusiieY; % Position plus confidence bound
rnmY = rY - plusiieY; % Position minus confidence bound

figure(3)
plot(t, rY, t, rnp,'-.', t, rnm,'-.');
xlabel('Time (s)')
ylabel('Position (m)')
title('Calculated Position Y and Time')

figure(2)
plot(abs(r), rY);
hold on;
plot([0 0.5], [0 0], 'Linewidth', 1.5);
xlabel('X Position (m)')
ylabel('Y Position (m)')
title('Calculated Position X and Y')
