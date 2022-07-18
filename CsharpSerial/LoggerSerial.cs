using System;
using System.IO.Ports;
using System.Text;
using System.Text.Json;


//List the serial ports:
public class LoggerSerial
{
    public struct Response
    {
        public string Command;
        public string Payload;
    }
    //COMMANDS:
    public const string CMD_FILEINFO = "!INFO";
    public const string CMD_READ_FILE = "!FILE";
    public const string CMD_READ_NEXT = "!NEXT";
    public const string CMD_ERASE_SD = "!ERASE";
    // public const string CMD_EPOCH = "!EPH0"; //Special command, 
    public const string CMD_LIST = "!LS";
    public const string CMD_PING = "!PING";
    public const string CMD_PONG = "!PONG";
    public const string CMD_CONFIRM = "!CONFIRM";

    public const string RESPONSE_CONFIRM = "!SURE?";
    public const string RESPONSE_NO_SYNC = "!Sync"; //Followed by an ascii-encoded int (1-8)
    public const int MESSAGELENGTH = 9; //Every message has a total of 9 bytes
    //Members:
    string mSelectedPort = "";
    public SerialPort mSerial = new SerialPort();


    Action<string>? customOnReceive = null;

    /// <summary>
    /// List all available serial ports
    /// </summary>
    public static string[] ListPorts()
    {
        //Console.WriteLine("--listing ports: ");
        string[] ports = SerialPort.GetPortNames();
        int i = 0;
        foreach (string s in ports)
        {
            Console.WriteLine("--{0}: {1}", i, s);
            i++;
        }
        return ports;
    }

    public string responseBuffer = "";

    /// <summary>
    /// Set the port of the serial object
    /// </summary>
    /// <param name="name">full name of the port (e.g. "COM5")</param>
    /// <returns></returns>
    public bool setPort(string name)
    {
        if (SerialPort.GetPortNames().Contains(name))
        {
            mSelectedPort = name;
            mSerial.PortName = name;
            this.init();
            return true;
        }
        return false;
    }

    /// <summary>
    /// Select a port through the console, when using as a CLI
    /// </summary>
    public void selectPortConsole()
    {
        LoggerSerial.ListPorts();
        //Console.WriteLine("--Select a port, write the full port name (e.g. \"COM3\")");
        string? selection = Console.ReadLine();
        //If invalid port name is typed:
        if (selection == null || !(selection.StartsWith("COM")))
        {
            //Console.WriteLine("--Invalid port typed");
            //try again:
            this.selectPortConsole();
        }
        //if portname is not a connected port:
        if (!SerialPort.GetPortNames().Contains(selection))
        {
            //Console.WriteLine("--Selected port not found");
            //try again:
            this.selectPortConsole();
        }

        //If port is OK, set it:
        mSelectedPort = selection == null ? "" : selection; //remove warning that selection could be null (even though it can not be)
        if (mSelectedPort != "") mSerial.PortName = mSelectedPort;
        this.init();
    }

    public string port
    {
        get { return mSelectedPort; }
    }

    /// <summary>
    /// Initialise the serial port, open, set baud and add event handler
    /// </summary>
    private void init()
    {
        mSerial.Open();
        if (mSerial == null) return;
        mSerial.BaudRate = 115200;
        mSerial.StopBits = System.IO.Ports.StopBits.One;
        mSerial.DataReceived += onDataReceive;
    }

    /// <summary>
    /// Event handler when the serial port receives data
    /// </summary>
    /// <param name="sender">The sending object</param>
    /// <param name="e">the event arguments</param>
    public void onDataReceive(object sender, SerialDataReceivedEventArgs e)
    {
        char[] recvBuffer = new char[mSerial.ReadBufferSize];
        Array.Clear(recvBuffer, 0, recvBuffer.Length);
        mSerial.Read(recvBuffer, 0, mSerial.ReadBufferSize);
        string message = new String(recvBuffer);
        byte[] bytes = Encoding.Default.GetBytes(message);
        message = Encoding.UTF8.GetString(bytes);



        if (message.StartsWith("!SYNC"))
        {
            char offsetChar = message["!SYNC".Length];
            int offset = offsetChar - '0';
            Synchronise(offset);
            Console.Write("Synchronizing...\n");
            return;
        }
        // if (message.StartsWith("!"))
        // {
        //     Console.Write(message);
        //     return;
        // }
        this.responseBuffer += message;

        if (customOnReceive != null) customOnReceive(message);
    }

    public void setHandler(Action<string> f)
    {
        customOnReceive = f;
    }


    /// <summary>
    /// Send a string as a command over the serial port
    /// </summary>
    /// <param name="cmd"> The command to send, can at most be {MESSAGELENGTH} long </param>
    public void sendCommand(string cmd)
    {
        //Max cmd length is 8 chars
        if (cmd.Length > MESSAGELENGTH)
        {
            //Console.WriteLine("--invalid command"); 
            return;
        }
        //If shorter than messageLength, pad with '0'
        while (cmd.Length < MESSAGELENGTH)
        {
            cmd += "0";

        }
        //Console.WriteLine("--Writing command {0}", cmd);
        mSerial.Write(cmd.ToCharArray(), 0, MESSAGELENGTH);
    }
    /// <summary>
    /// For testing/debugging, automatically send a message of the wrong length to desync the devices
    /// </summary>
    /// <param name="spaces"> the amount of characters to send </param>
    public void intentionalDesync(int spaces)
    {
        if (spaces < 1 || spaces > 7) return;
        string message = "";
        for (int i = 0; i < spaces; i++)
        {
            message += "0";
        }

        //Console.WriteLine("--Writing desync: {0} ({1})", message, message.Length);
        mSerial.Write(message.ToCharArray(), 0, message.Length);
    }
    /// <summary>
    /// Re-sync with the device
    /// </summary>
    /// <param name="count"> The amount of spaces to sync </param>
    private void Synchronise(int count)
    {
        //Count received is the location the '!' was received in, need to sync by sending "Messagelength - count"
        for (int i = count; i < MESSAGELENGTH; i++)
        {
            mSerial.Write("0".ToCharArray(), 0, 1);
        }
    }

    /// <summary>
    /// Reads all contents of the response buffer and clear it
    /// </summary>
    /// <returns>string containing response data</returns>
    public string readBuffer()
    {
        string currentBuffer = responseBuffer;
        responseBuffer = "";
        return currentBuffer;
    }

    /// <summary>
    /// Read all contents of the response buffer, clear it and split per-message
    /// </summary>
    /// <returns>Array of strings containing each message</returns>
    public string[] getResponses()
    {
        string totalResponses = this.readBuffer();
        return totalResponses.Split("\n");
    }

    /// <summary>
    /// Send the current UNIX timestamp to the device
    /// </summary>
    public void sendEpoch()
    {
        char[] cmdStr = "!EPH0".ToCharArray();
        byte[] cmd = Encoding.UTF8.GetBytes(cmdStr);

        long time = DateTimeOffset.Now.ToUnixTimeSeconds();
        //Casting to 32-bit int, will limit our timespan (2038) but makes it fit in a message
        //Update message length on device and in API to make it 64-bit compatible
        int intTime = ((int)time);
        //Console.WriteLine(String.Format("UNIX: {0}, HEX: 0x{1:X}", intTime, intTime));

        byte[] timeBytes = BitConverter.GetBytes(intTime);
        //Console.WriteLine(String.Format("0x{0:X} {1:X} {2:X} {3:X} --- {4}", timeBytes[0], timeBytes[1], timeBytes[2], timeBytes[3], timeBytes.Length));
        byte[] command = cmd.Concat(timeBytes).ToArray();
        // Console.WriteLine(String.Format("{0}{1}{2}{3}{4} {5:X}{6:X}{7:X}{8:X}",
        // (char)command[0], (char)command[1], (char)command[2], (char)command[3], (char)command[4],
        //                 command[5], command[6], command[7], command[8]));
        ////Console.WriteLine("--Writing command {0}", cmd);
        mSerial.Write(command, 0, MESSAGELENGTH);
    }
    
    /// <summary>
    /// Parse a message string into a seperate command type and payload
    /// </summary>
    /// <param name="msg"></param>
    /// <returns>Response data in struct</returns>
    public static Response Parse(string msg)
    {
        Response response = new Response();
        //Response message example: "cmd=!FILE/payload={"Filesize":259, "packetcount":3}"
        try
        {
            string[] kvPairs = msg.Split("/"); //Split message into seperate key-value pairs
            response.Command = kvPairs[0].Split("=")[1];
            response.Payload = kvPairs[1].Split("=")[1];
        }
        catch
        {
            response.Command = "";
            response.Payload = "";
        }
        return response;
    }
}
