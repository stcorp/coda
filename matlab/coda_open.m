function varargout = coda_open(varargin)
% CODA_OPEN  Open a product file.
%
%   CODA_FILE_ID = CODA_OPEN(FILEPATH) opens the product file pointed to
%   by filepath. Upon successful opening the function returns a file
%   handle that should be passed to other CODA MATLAB interface functions
%   that need to read data/information from this product file.
%
%   The filepath parameter must be a string containing the full path (or
%   relative path with respect to the current working directory) of the
%   product file.
%
%   See also CODA_CLOSE, CODA_OPEN_AS, CODA_CLEARALL
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('OPEN',varargin{:});
