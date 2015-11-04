function varargout = coda_setopt(varargin)
% CODA_SETOPT  Set CODA-MATLAB options.
%
%   CODA_SETOPT(OPTION_NAME, VALUE) assigns VALUE to the option specified
%     by OPTION_NAME.
%
%   See also CODA_GETOPT, CODA_OPTIONS
%

% Call CODA_MATLAB.MEX to do the actual work.
coda_matlab('SETOPT',varargin{:});
