% Steinhart_hart_fitting_proj

%% Non-linear data fitting. This script demonstrates several techniques
%  for fitting data sets with curves other than lines, calculating the
%  confidence intervals on the fitted parameters, and calculating and
%  displaying the functional confidence bounds and the observational
%  bounds. The data set is take from the data sheet for the TCS10K5 NTC
%  thermistor, which lists the resistance as a function of temperature
%  in degrees C. Since we are fitting the Steinhart-Hart equation to the
%  data, we first converted the temperature to Kelvin. Next, because the
%  Steinhart-Hart equation has T as a function of R, we use T or 1/T as y
%  and R or ln(R) as x. Finally, because the data are so clean, we add
%  noise of ą5% to the T data for some of the fits.

% The temperatures. (kelvin)
T = [281.15, 282.05, 283.05, 284.25, 285.15, 286.25, 287.15, 288.05, 288.95, 290.15, 291.25, 292.25, 292.85, 293.95, 294.95];
% The resistances.
R = [182805.43, 142365.49, 124315.84, 102593.19, 80180.18, 58831.00, 51561.08, 40924.46, 30174.43, 20540.02, 11706.88, 6382.98, 2249.49, 40.02, 20.00];
% The noisy temperatures.
TN = [281.15, 282.05, 283.05, 284.25, 285.15, 286.25, 287.15, 288.05, 288.95, 290.15, 291.25, 292.25, 292.85, 293.95, 294.95];
confLev = 0.95; % We set the confidence level for the data fits here.

%% Plot Original Data
% Plot the original temperature and noisy temperature data.
figure(1) 
plot(R,T)
hold on
plot (R,TN,'x')
xlabel('Resistance (\Omega)')
ylabel('Temperature (K)')
title('Original and Noisy Data')
legend('Original Temperatures','Noisy Temperatures')
hold off

% Since a plot of 1/T vs ln(R) should be close to linear, we will convert
% the data to the correct forms and do linear and polynomial fits with
% them.
ooT = 1./T;
ooTN = 1./TN;
lnR = log(R);

%% Plot Transformed Data
% Plot the transformed original temperature and noisy temperature data.
figure(2) 
plot(lnR,ooT)
hold on
plot (lnR,ooTN,'x')
xlabel('ln[Resistance (\Omega)]')
ylabel('Reciprocal Temperature (1/K)')
title('Original and Noisy Data Transformed')
legend('Original Temperatures','Noisy Temperatures')
hold off

% Start by working with the original transformed data. We will fit a 1st,
% 2nd, and 3rd order polynomial to the data and look at the fit and the
% residuals.
range = max(lnR) - min(lnR); % Get range for our xplot data
xplot = min(lnR):range/30:max(lnR); % Generate x data for some of our plots.
% The fitting routine 'fit' is particular about the form of the data.
% Use the line below to get your data into the correct format for 'fit'.
[Xout,Yout] = prepareCurveData(lnR, ooT); 
[f1,stat1] = fit(Xout,Yout,'poly1') % 1st-order fit with statistics.
[f2,stat2] = fit(Xout,Yout,'poly2') % 2nd-order fit with statistics.
[f3,stat3] = fit(Xout,Yout,'poly3') % 3rd-order fit with statistics.
% The default for confidence level is 0.95. If we wanted the confidence
% intervals on the parameters for a value of confLev other than 0.95, we
% would uncomment the following lines:
% format short e
% ci1 = confint(f1,confLev)
% ci2 = confint(f2,confLev)
% ci3 = confint(f3,confLev)
% format

%% Plot 1st-order Fit
% Plot data with 1st-order fit. Since the data have very little noise (just
% round-off or truncation error), the data and the curve will vary but
% little.
figure(3)
plot(f1,Xout,Yout) % Notice that the fit doesn't need both x and y values.
% It will plot with just the f1 object.
xlabel('ln[Resistance (\Omega)]')
ylabel('Reciprocal Temperature (1/K)')
title('1st-Order Fit to Transformed Data')

%% Plot Residuals
% Since the fit is so close, we need a better way to distinguish how well
% 1st-, 2nd-, and 3rd-order fit. We'll plot the residuals to compare.
figure(4)
subplot(3,1,1)
plot(f1,Xout,Yout,'residuals')
% xlabel('Ln[Resistance (\Omega)]')
ylabel('Residuals (1/K)')
title('1st Order Polynomial Fit')
% The linear residuals look parabolic, so we need at least 2nd order.
subplot(3,1,2)
plot(f2,Xout,Yout,'residuals')
% xlabel('Ln[Resistance (\Omega)]')
ylabel('Residuals (1/K)')
title('2nd Order Polynomial Fit')
% The 2nd-order residuals look cubic so we need at least 3rd order.
subplot(3,1,3)
plot(f3,Xout,Yout,'residuals')
xlabel('Ln[Resistance (\Omega)]')
ylabel('Residuals (1/K)')
title('3rd Order Polynomial Fit')
% The 3rd-order residuals look pretty random, so we're done.

%% Plot 3rd-order Fit and Bounds
% Let's plot the 3rd order fit with the data and the functional and
% observational bounds. Note, since there is so little noise in the data,
% we won't be able to distinguish which is which.
p11 = predint(f3,xplot,confLev,'observation','off'); % Gen conf bounds
p21 = predint(f3,xplot,confLev,'functional','off'); % Gen conf bounds
figure(5)
plot(f3,Xout,Yout) % Notice that the fit doesn't need both x and y.
hold on
plot(xplot, p21, '-.b') % Upper and lower functional confidence limits
plot(xplot, p11, '--m') % Upper and lower observational confidence limits
xlabel('Ln[Resistance (\Omega)]')
ylabel('Reciprocal Temperature (1/K)')
title('3rd Order Polynomial Fit')
legend('Data Points','Best Fit Line','Upper Func. Bound',...
    'Lower Func. Bound', 'Upper Obs. Bound', 'Lower Obs. Bound',...
    'Location', 'northwest')
hold off

%% 3rd-order with Noise
% Since the original data have so little noise, let's repeat the 3rd order
% plot for the noisy data. We'll reuse some of the variables
[Xout,Yout] = prepareCurveData(lnR, ooTN); 
[f3,stat1] = fit(Xout,Yout,'poly3') % 1st-order fit with statistics.
p11 = predint(f3,xplot,confLev,'observation','off'); % Gen conf bounds
p21 = predint(f3,xplot,confLev,'functional','off'); % Gen conf bounds
figure(6)
plot(f3,Xout,Yout) % Notice that the fit doesn't need both x and y.
hold on
plot(xplot, p21, '-.b') % Upper and lower functional confidence limits
plot(xplot, p11, '--m') % Upper and lower observational confidence limits
xlabel('Ln[Resistance (\Omega)]')
ylabel('Reciprocal Temperature (1/K)')
title('3rd Order Polynomial Fit, Noisy Data')
legend('Data Points','Best Fit Line','Upper Func. Bound',...
    'Lower Func. Bound', 'Upper Obs. Bound', 'Lower Obs. Bound',...
    'Location', 'northwest')
hold off

%% Transform Fit Back
% Let's untransform the best fit and confidence bounds back into the
% original space with T vs R and see how good we feel about things.
yplot = f3(xplot);
figure(7)
plot(R,TN,'x')
hold on
plot(exp(xplot), 1./yplot)
plot(exp(xplot), 1./p21, '-.b') % Upper and lower functional confidence limits
plot(exp(xplot), 1./p11, '--m') % Upper and lower observational confidence limits
legend('Data Points','Best Fit Line','Upper Func. Bound',...
    'Lower Func. Bound', 'Upper Obs. Bound', 'Lower Obs. Bound',...
    'Location', 'northeast')
xlabel('Resistance (\Omega)')
ylabel('Temperature (K)')
title('Retransformed 3rd-Order with Fit Lines')
hold off

%% Nonlinear Fit
% To compare this transformed linear fit of a polynomial with non-linear, let's
% do a non-linear fit using the Steinhart-Hart equation (but we'll include
% the 2nd-order term.
range = max(R) - min(R); % Get range for our xplot data
xplot = min(R):range/30:max(R); % Generate x data for some of our plots.
% First we have to define the function we will fit.
% Things work better if we have starting points for a, b, c, and d. We'll
% use our values from above and 'fitoptions'
fo = fitoptions('Method','NonlinearLeastSquares',...
    'StartPoint',[-0.002894 0.001339 -9.963e-05 3.053e-06]);
ft = fittype('1/(a+b*log(R)+c*(log(R)^2)+d*(log(R)^3))','independent',...
    'R','options',fo);
% Next, we have to get our data into the correct format for 'fit'.
[Xout,Yout] = prepareCurveData(R, TN);
% Now we'll do our fit.
[f4,stat4] = fit(Xout,Yout,ft)
p11 = predint(f4,xplot,confLev,'observation','off'); % Gen conf bounds
p21 = predint(f4,xplot,confLev,'functional','off'); % Gen conf bounds
figure(8)
plot(f4,Xout,Yout) % Notice that the fit doesn't need both x and y.
hold on
plot(xplot, p21, '-.b') % Upper and lower functional confidence limits
plot(xplot, p11, '--m') % Upper and lower observational confidence limits
xlabel('Resistance (\Omega)')
ylabel('Temperature (K)')
title('Steinhart-Hart Fit, Noisy Data')
legend('Data Points','Best Fit Line','Upper Func. Bound',...
    'Lower Func. Bound', 'Upper Obs. Bound', 'Lower Obs. Bound',...
    'Location', 'northeast')
hold off
% In many cases, the non-linear least squares fit looks better than the
% transformed-fit-untransformed fit. This is a rare case when it doesn't

%% Nonlinear Residuals
% And finally, the residuals
figure(9)
plot(f4,Xout,Yout,'residuals')
xlabel('Resistance (\Omega)')
ylabel('Residuals (K)')
title('Steinhart-Hart Fit')