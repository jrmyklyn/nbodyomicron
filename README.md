# nbodyomicron
N-body simulation
=================

Usage:

`nbody data_input.csv [options]`

**OPTIONS**

	-t
		Time slice (default = 1 hour) - Seconds passed between n-body calulations
	-u
		Updates per second (default = 100) - Number of times to update n-body calculations per second (if calculations take longer than this parameter, the simulation will run as quickly as possible and the parameter is ignored)
	-l
		Minimum radius of the largest body relative to the display size
	-s
		Minimum radius of the smallest body relative to the display size
