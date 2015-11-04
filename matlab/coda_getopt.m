function varargout = coda_getopt(varargin)
% CODA_GETOPT  Retrieve CODA MATLAB interface options.
%
%   OPTIONS = CODA_GETOPT returns a record with all available CODA MATLAB
%     interface options and their current values.
%
%   VALUE = CODA_GETOPT(OPTION_NAME) will return the value of the
%     specified option.
%
%   See also CODA_SETOPT, CODA_OPTIONS
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('GETOPT',varargin{:});
