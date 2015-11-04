function varargout = coda_class(varargin)
% CODA_CLASS  Return object class.
%
%   C = CODA_CLASS(CODA_FILE_ID, <DATA SPEC ARGS>) returns the MATLAB
%   class of the specified data element. With coda_class you can check
%   what the class of a data element will be without the need to
%   retrieve it via coda_fetch. The returned class is identical to what
%   would be returned by class(coda_fetch(...)) if coda_class and
%   coda_fetch had identical parameters.
%
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open. The format for the data specification
%   argument list <data spec args> is described in the help page for
%   coda_param.
%
%   See also CODA_FETCH, CODA_OPEN, CODA_PARAM
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('CLASS',varargin{:});
