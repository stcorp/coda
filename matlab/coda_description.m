function varargout = coda_description(varargin)
% CODA_DESCRIPTION  Retrieve field description.
%
%   DESC = CODA_DESCRIPTION(CODA_FILE_ID, <DATA SPEC ARGS>) returns a
%   string containing the description in the data dictionary of the
%   specified data element. If the last item of the data specifier
%   argument list equals a fieldname then you will get the description
%   from the Data Dictionary for this field.
%
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open. The format for the data specification
%   argument list <data spec args> is described in the help page for
%   coda_param.
%
%   See also CODA_FETCH, CODA_OPEN, CODA_PARAM
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('DESCRIPTION',varargin{:});
