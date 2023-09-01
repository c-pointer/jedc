% synchronous process support

%_debug_info = 1;
private variable Filter_Processes = NULL;

private define find_filter_process (pid)
{
   variable s = Filter_Processes;
   while (s != NULL)
     {
	if (s.pid == pid)
	  break;
	s = s.next;
     }

   return s;
}

private define delete_filter_process (s)
{
   if (Filter_Processes == NULL)
     return;

   if (Filter_Processes == s)
     {
	Filter_Processes = s.next;
	s.next = NULL;
	return;
     }

   variable prev = Filter_Processes;
   variable next = prev.next;

   while (next != s)
     {
	if (next == NULL)
	  return;

	prev = next;
	next = next.next;
     }

   prev.next = next.next;
   s.next = NULL;
}

private define process_signal_handler (pid, flags, status)
{
   if (flags == 1)
     return;

   variable s = find_filter_process (pid);
   if (s == NULL)
     return;

   if (flags == 4)
     s.exit_status = status;
   else
     s.exit_status = -1;
}

private define allocate_filter_process (pid)
{
   variable s = struct
     {
	pid, exit_status,
	  next
     };

   s.pid = pid;
   s.next = Filter_Processes;
   Filter_Processes = s;
   return s;
}

%!%+
%\function{open_filter_process}
%\synopsis{Open a subprocess as a filter}
%\usage{Int_Type pid = open_filter_process (String_Type argv[], String_Type output)}
%\description
% The \var{open_filter_process} function may be used to open an interactive
% synchronous process.  The first argument should be an array of strings
% representing the program to be run in the subprocess, and the command line
% parameters passed to it.  The second argument specifies what to do with the
% output generated by the process.  It can be any value supported by the
% "output" option of the \var{set_process} function.  The process should be
% closed using the \var{close_filter_process} function.
%\seealso{close_filter_process, send_process, call_process_region}
%!%-
public define open_filter_process (argv, output)
{
   if (typeof (argv) != Array_Type)
     argv = [argv];

   variable nargs = length (argv);
   foreach (argv)
     ;
   variable args = __pop_args (nargs);

   variable pgm = argv[0];

   variable pid = open_process_pipe (__push_args (args), nargs-1);
   if (-1 == pid)
     verror ("failed to execute %s", pgm);

   variable s = allocate_filter_process (pid);

   set_process (pid, "signal", &process_signal_handler);
   set_process (pid, "output", output);

   return pid;
}

%!%+
%\function{close_filter_process}
%\synopsis{Close a filter process and return its status}
%\usage{Int_Type close_filter_process (Int_Type pid)}
%\description
% The \var{close_filter_process} function waits for the specified process
% to terminate and returns the exit status of the process.  The process must
% have been previously opened via the \var{open_filter_process} function.
%\seealso{open_filter_process, send_process, get_process_input}
%!%-
public define close_filter_process (pid)
{
   variable s = find_filter_process (pid);
   if (s == NULL)
     return -1;

   %send_process_eof (pid);
   get_process_input (0);

   variable next_signal = 2;
   while (s.exit_status == NULL)
     {
	update (0);
	ERROR_BLOCK
	  {
	     signal_process (pid, next_signal);
	     if (next_signal != 9)
	       _clear_error ();
	     next_signal = 9;
	  }
	get_process_input (5);
     }

   variable status = s.exit_status;
   delete_filter_process (s);
   return status;
}

public define call_process_region (cmd, output)
{
   variable str = bufsubstr ();
   variable pid = open_filter_process (cmd, output);
   if (pid == -1)
     return;

   send_process (pid, str);
   send_process_eof (pid);
   vmessage ("Process returned %d", close_filter_process (pid));
}

