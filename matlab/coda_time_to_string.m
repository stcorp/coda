function varargout = coda_time_to_string(varargin)
% CODA_TIME_TO_STRING  Convert a number of seconds since 1-1-2000 to a
%   human-readable string format.
%
%   STR = CODA_TIME_TO_STRING(N_SECONDS_SINCE_2000) turns the double
%   value containing the amount of seconds since 1-1-2000 into a string
%   containing the date and time in a human readable string of the form
%   '2000-01-01 00:00:00.000000'.
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('TIME_TO_STRING',varargin{:});
