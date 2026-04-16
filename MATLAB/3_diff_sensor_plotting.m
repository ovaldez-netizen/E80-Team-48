% Three_diff_sensor_plotting

%% Convert all points to teensy units (only for y bc for x it it is just
%% sample number)

% Conversion: = 0.9V/278Tu = 0.00324 V per Teensy Unit [from lab 2]
pressure = 0.00324 * double(A00) * (-2.4) + 7.97 % conversion based on calibration
thermistor = double(A01) * 0.00324 * (-0.391) + 20.6
PAR = 558 * double(A02) * 0.00324 * 558 - 26.3


% Teensy sampling frequency = 10 Hz
% 10 samples per second
t_pr = [1: 10 * length(A00)] % A00 pressure
t_thr = [1: 10 * length(A01)] % A01 thermistor
t_pho = [1: 10 * length(A02)] % A02 PAR

% Pressure: Call: A00 /////////////////////////////////////////////////////

// plot(pressure, )
//xlabel("Time (s)")
//ylabel("Pressure (V)")
//title("Pressure Sensor Data")

% Thermistor: Call: A01 ///////////////////////////////////////////////////

    % Plot ___________________________________________________________
plot(pressure, thermistor)
xlabel("Depth (m)")
ylabel("Temp (C)")
title("Thermistor Data")




%% Photodiode: Call: A02 ///////////////////////////////////////////////////
% photodiode outputs a voltage. 
 

% create equation to convert values to light intensity (lux)
% with calibration curve (should be linear)

% '1' specifies a first-degree polynomial (linear fit: y = mx + b)
p = polyfit(t_pho, PAR, 1); 




//PARlux = [];
//or i = 1:length(PAR);
   // L = p(1)*t_pho + p(2)
   //% Append
    //PARlux[end+1] = L;
//end



% plot final values
plot(pressure, PAR)
xlabel("Depth (m)")
ylabel("PAR (lux)")
title("PAR Photodiode Data vs. Time ")