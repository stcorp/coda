function varargout = coda_clearall(varargin)
% CODA_CLEARALL  Close all open product files and remove data dictionary
%   from memory.
%
%   CODA_CLEARALL will close all currently open product files (similar
%   to calling coda_close for all open product files). This can be
%   especially helpful to close files for which you have lost the file
%   handle (something that can occur if you were running an M file that
%   opened a product file, but terminated halfway and thus left the file
%   open).
%
%   This function will also free all product format definitions from
%   memory. When coda_open is called, CODA will read the product format
%   definition for the opened file and keeps this cached in memory.
%   A call to coda_clearall will remove all cached definitions.
%
%   See also CODA_OPEN, CODA_CLOSE
%

% Call CODA_MATLAB.MEX to do the actual work.
coda_matlab('CLEARALL',varargin{:});
