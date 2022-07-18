using System;
using System.Threading;
using System.IO.Ports;
using System.Text.Json;
using System.Text.Json.Nodes;

class Program
{
    /// <summary>
    /// a simple CLI to demonstrate the use of the loggerSerial API
    /// </summary>
    /// <param name="args"></param>
    public static void Main(string[] args)
    {

        LoggerSerial serialPort = new LoggerSerial();

        //Set serialport to arg0 if it was given
        if (args.Length > 0)
        {
            if (!serialPort.setPort(args[0]))
            {
                //If arg0 port was invalid, ask through CLI
                serialPort.selectPortConsole();
            }
        }
        //if no arg was given, ask through CLI
        else { serialPort.selectPortConsole(); }

        //Optionally set a custom handler that gets called automatically when serial data is received (callback)
        serialPort.setHandler(onResponseReceived);

        Console.WriteLine("--Connecting to {0}", serialPort.port);
        while (true)
        {
            string? read = Console.ReadLine();
            if (read != null)
            {
                switch (read.ToUpper())
                {
                    case "READ":
                        serialPort.sendCommand(LoggerSerial.CMD_FILEINFO);
                        break;
                    case "ERASE":
                        serialPort.sendCommand(LoggerSerial.CMD_ERASE_SD);
                        break;
                    case "LS" or "LIST":
                        serialPort.sendCommand(LoggerSerial.CMD_LIST);
                        break;
                    case "PING":
                        serialPort.sendCommand(LoggerSerial.CMD_PING);
                        break;
                    case "FILE":
                        serialPort.sendCommand(LoggerSerial.CMD_READ_FILE);
                        break;
                    case "NEXT":
                        serialPort.sendCommand(LoggerSerial.CMD_READ_NEXT);
                        break;
                    case "TIME":
                        serialPort.sendEpoch();
                        break;
                    case "NEW":
                        serialPort.sendCommand("!NEW");
                        break;
                    case "FULL":
                        serialPort.sendCommand(LoggerSerial.CMD_FILEINFO);
                        Thread.Sleep(100);
                        serialPort.sendCommand(LoggerSerial.CMD_READ_FILE);
                        Thread.Sleep(100);
                        serialPort.sendCommand(LoggerSerial.CMD_READ_NEXT);
                        Thread.Sleep(100);
                        serialPort.sendCommand(LoggerSerial.CMD_READ_NEXT);
                        Thread.Sleep(100);
                        serialPort.sendCommand(LoggerSerial.CMD_READ_NEXT);
                        Thread.Sleep(100);
                        break;
                    case "RESPONSE":
                        string[] responses = serialPort.getResponses();
                        int i = 0;
                        if (responses[0] != "")
                        {
                            foreach (string response in responses)
                            {
                                Console.WriteLine(String.Format("Response {0}: {1}", i, response));
                                i++;
                            }
                        }
                        else Console.WriteLine("No responses available");
                        break;
                    default: break;
                }
            }
            Console.WriteLine();//newline
        }
        //Add a custom handler to a received message:
        //Callback function needs to have one string input as argument:
        void onResponseReceived(string s)
        {
            //Complete messages end on a newline:
            if (serialPort.responseBuffer.Contains("\n"))
            {
                string[] responses = serialPort.getResponses();

                foreach (string responseStr in responses)
                {
                    if (responseStr.Length > 0)
                    {

                        //Format: "cmd=!INFO/payload=This is a payload"
                        LoggerSerial.Response response = LoggerSerial.Parse(responseStr);
                        if (response.Command != "")
                        {
                            Console.WriteLine($"Command {response.Command}, Message {response.Payload}");
                        }
                    }
                }
            }

        }
    }

}