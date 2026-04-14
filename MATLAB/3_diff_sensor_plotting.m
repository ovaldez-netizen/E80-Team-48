% Three_diff_sensor_plotting

%% Convert all points to teensy units (only for y bc for x it it is just
%% sample number)

% Conversion: = 0.9V/278Tu = 0.00324 V per Teensy Unit [from lab 2]
pressure = A00 .* 0.00324
thermistor = A01 .* 0.00324
PAR = A02 .* 0.00324


% Teensy sampling frequency = 10 Hz
% 10 samples per second
t_pr = [1: 10 * length(A00)] % A00 pressure
t_thr = [1: 10 * length(A01)] % A01 thermistor
t_pho = [1: 10 * length(A02)] % A02 PAR

% Pressure: Call: A00 /////////////////////////////////////////////////////

plot(pressure, t_pr)
xlabel("Time (s)")
ylabel("Pressure (V)")
title("Pressure Sensor Data")

% Thermistor: Call: A01 ///////////////////////////////////////////////////

% Calibrate for Coeff of Steinhart-hart _______________________________
A = 0.003246
B = 8.163e-05
C = -1.332e-05
D = 7.202e-07

    % Temp Calc: __________________________________________________________
temp = [];
for i = 1:length(thermistor);
    % Equation
    T = ( A + B*(ln(thermistor(:, i))) + C*(ln(thermistor(:, i)))^2 )+ D*(ln(thermistor(:, i)))^3 )
    % Append
    temp[end+1] = T;
end



    % Plot ___________________________________________________________
plot(temp, t_thr)
xlabel("Time (s)")
ylabel("Temp (C)")
title("Thermistor Data")




%% Photodiode: Call: A02 ///////////////////////////////////////////////////
% photodiode outputs a voltage. 
 

% create equation to convert values to light intensity (lux)
% with calibration curve (should be linear)

% '1' specifies a first-degree polynomial (linear fit: y = mx + b)
p = polyfit(t_pho, PAR, 1); 




PARlux = [];
for i = 1:length(PAR);
    L = p(1)*t_pho + p(2)
    % Append
    PARlux[end+1] = L;
end



% plot final values
plot(PAR, t_pho)
xlabel("Time (s)")
ylabel("PAR (lux)")
title("PAR Photodiode Data vs. Time ")