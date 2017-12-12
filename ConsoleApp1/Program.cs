using System;
using System.Collections.Generic;

namespace ConsoleApp1
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var output = string.Join(", ", args);
            Console.WriteLine(output);
        }
        public static int Start(string arguments)
        {
            var arglist = new List<string>();
            for (int i = 0; i < arguments.Length; i++)
            {
                var first = arguments.IndexOf('"', i);
                var last = arguments.IndexOf('"', i + 1);
                var len = last - first - 1;
                arglist.Add(arguments.Substring(first + 1, len));
                i += len + 2;
            }
            Main(arglist.ToArray());

            return arglist.Count;
        }
    }
}
