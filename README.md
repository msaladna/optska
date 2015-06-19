This is a fix to the problem with GetTicks() under
Windows which returns a DWORD only allocating
4 bytes of storage, so if your uptime is
greater than 2^32/1000 seconds (ticks are in millisecs)
then it experiences a rollover.  I guess Windows
developers never considered a non WinXP/2k/NT box to
have any uptime greater than 49.7 days.

At any rate this returns the seconds elapsed since
the last reboot through the performance counter
and there are no sanity checks as to whether you have
the performance counter dll present (WinXP/NT/2k) or
not... it's on the to-do list, as is future improvements
to give XiRCON functionality to obtain other counter
information.

Call the proc under XiRCON as 'opts' and from there,
if using kano, you can run it by 'since', e.g.:
set uptime [since [opts]]
Which will return something along the lines of:
[ka] Tcl: 2mn 4d 15h 51m 1s

This fix returns a 64 bit wide int (LONGLONG) that'll
guarantee accurate non-rollover uptime reports for
up to 2^64 seconds, or 584942417355.07203247 years...
I don't think I'll need to write another patch
any time soon.

Do not call opts for any high-resolution benchmarking
such as the time elapsed during an interval where the
resolution needs to be high.  The value returned is in
seconds and definitely _not_ milliseconds (as stated
before).  Also too because of this property, you don't
need to divide the value by 1000 to get seconds, that'd
be what, kiloseconds?  Well something odd.

There's a slight start-up cost though when the query
is first instantiated when calling opts for the first
time, but remains resident for subsequent calls--
that's why kano will hang for a second or two on
/uptime.  

The opts addon does give some interesting ideas to add
some more nifty functionality such as the ability to 
add and query performance counters, but that'll come
around when I have time.

To install, just extract the opts.dll and opts.ka to
the addons directory of kano then pop open kano.tcl
in your favorite text editory (XeD works here) and
find:
```
if {[osuptime] > [get_cookie winuptime 0]} {set_cookie winuptime \
        [osuptime]}
```        
within the minute hook (approximately line 9212), change to:
```
if {[opts] > [get_cookie winuptime 0]} {set_cookie winuptime \
        [opts]}
```        
that should fix the best os uptime and /reload - done.

- Matt Saladna
msaladna@apisnetworks.com
