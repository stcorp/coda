function varargout = coda_eval(varargin)
% CODA_EVAL  Evaluate a CODA expression.
%
%   RESULT = CODA_EVAL(CODA_EXPRESSION, [CODA_FILE_ID, <DATA SPEC ARGS>])
%   returns the evaluated CODA expression at the given product location.
%   The coda_expression parameter should be a string with a valid CODA
%   expression. The syntax for the CODA expression language can be found in the
%   CODA Expressions documentation.
%
%   If the CODA expression can be evaluated statically, then coda_file_id and
%   <data spec args> are not required, otherwise these arguments are mandatory.
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open. The format for the data specification
%   argument list <data spec args> is described in the help page for
%   coda_param.
%
%   Note that evaluation of 'void' expressions is not supported.
%
%   See also CODA_FETCH, CODA_OPEN, CODA_PARAM
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('EVAL',varargin{:});
