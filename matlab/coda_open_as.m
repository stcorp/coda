function varargout = coda_open_as(varargin)
% CODA_OPEN_AS  Open a product file using a specific format definition.
%
%   CODA_FILE_ID = CODA_OPEN_AS(FILEPATH, PRODUCT_CLASS, PRODUCT_TYPE, VERSION)
%   will try to open the specified file for reading similar to CODA_OPEN, but
%   instead of trying to automatically recognise the applicable product
%   class/type/version as CODA_OPEN does, this function will impose the format
%   definition that is associated with the given product_class, product_type,
%   and version parameters.
%
%   Note that you normally won't need this function as CODA will be able to
%   recognize which format definition to use automatically. However, for the
%   rare occasions where CODA_OPEN is not sufficient, you can use this function
%   to force the use of a specific format definition.
%
%   You can specify -1 for the version to request the latest available version
%   of the format definition.
%
%   See also CODA_OPEN, CODA_CLOSE, CODA_CLEARALL
%

% Call CODA_MATLAB.MEX to do the actual work.
[varargout{1:max(1,nargout)}] = coda_matlab('OPEN_AS',varargin{:});
