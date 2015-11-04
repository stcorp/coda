function varargout = coda_close(varargin)
% CODA_CLOSE  Close an open product file.
%
%   CODA_CLOSE(CODA_FILE_ID) closes the product file which is associated
%   with the coda_file_id file handle.
%
%   The coda_file_id parameter must be a valid CODA file handle that was
%   retrieved with coda_open.
%
%   See also CODA_OPEN, CODA_CLEARALL
%

% Call CODA_MATLAB.MEX to do the actual work.
coda_matlab('CLOSE',varargin{:});
