function varargout = coda_attributes(varargin)
% CODA_ATTRIBUTES  Return object attributes.
%
%   C = CODA_ATTRIBUTES(CODA_FILE_ID, <DATA SPEC ARGS>) returns a struct
%   containing all attributes that are associated with the specified data
%   element. If no attributes are available an empty struct will be
%   returned.
%
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open. The format for the data specification
%   argument list <data spec args> is described in the help page for
%   coda_param.
%
%   See also CODA_FETCH, CODA_OPEN, CODA_PARAM
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('ATTRIBUTES',varargin{:});
