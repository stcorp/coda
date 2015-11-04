function varargout = coda_fieldcount(varargin)
% CODA_FIELDCOUNT  Retrieve the number of fields for a record in a
%   product file.
%
%   N = CODA_FIELDCOUNT(CODA_FILE_ID, <DATA SPEC ARGS>) returns the
%   number of fields for the struct that would be returned if coda_fetch
%   would have been called with the same arguments. The last item of the
%   data specifier argument should point to a record.
%
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open. The format for the data specification
%   argument list <data spec args> is described in the help page for
%   coda_param.
%
%   See also CODA_FETCH, CODA_OPEN, CODA_PARAM
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('FIELDCOUNT',varargin{:});
