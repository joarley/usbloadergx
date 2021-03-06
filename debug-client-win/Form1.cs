﻿using DebugOutput.Properties;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace DebugOutput
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            start.Enabled = true;
            stop.Enabled = false;
            textBox1.Enabled = true;

            if (!string.IsNullOrEmpty(Settings.Default.LastIp))
                textBox1.Text = Settings.Default.LastIp;

            Task.Run(Main);
        }

        enum ParseState
        {
            NotInitilized,
            Initialized,
            IdFound,
            MessageFound,
            EndFound
        }

        private Task Main()
        {
            TcpClient client = null;

            while (true)
            {
                try
                {
                    Invoke(new Action(() => { label1.Text = "Desconnected"; label1.ForeColor = Color.Red; }));

                    if (start.Enabled)
                    {
                        Thread.Sleep(500);
                        continue;
                    }


                    if (client != null)
                        client.Close();

                    var ip = textBox1.Text.Split(':')[0];
                    var port = int.Parse(textBox1.Text.Split(':')[1]);
                    client = new TcpClient();
                    client.NoDelay = true;
                    var taskConnect = client.ConnectAsync(ip, port);
                    taskConnect.Wait(2000);
                    if (!client.Connected)
                    {
                        client.Close();
                        continue;
                    }

                    Invoke(new Action(() => { label1.Text = "Connected"; label1.ForeColor = Color.Green; }));

                    List<string> workingPieces = new List<string>();
                    ParseState state = ParseState.NotInitilized;

                    while (stop.Enabled)
                    {
                        var stream = client.GetStream();

                        byte[] buffer = new byte[5000];

                        var readTask = stream.ReadAsync(buffer, 0, buffer.Length);
                        if (!readTask.Wait(2000))
                            continue;

                        if (readTask.Result == 0)
                            break;

                        var dataReceived = Encoding.UTF8.GetString(buffer).TrimEnd('\0');
                        var piecesReceived = dataReceived.Split(new[] { "%&_" }, StringSplitOptions.None);

                        if (string.IsNullOrEmpty(dataReceived) || !piecesReceived.Any())
                        {
                            Thread.Sleep(300);
                            continue;
                        }

                        foreach (var piece in piecesReceived)
                        {
                            if (piece == "INIT")
                            {
                                if (state != ParseState.NotInitilized)
                                    workingPieces.Clear();
                                workingPieces.Add(piece);
                                state = ParseState.Initialized;
                            }
                            else if (piece == "END")
                            {
                                if (state == ParseState.MessageFound)
                                {
                                    workingPieces.Add(piece);
                                    ProcessMessage(workingPieces.ToArray());
                                }

                                state = ParseState.NotInitilized;
                                workingPieces.Clear();
                            }
                            else if (state == ParseState.Initialized)
                            {
                                workingPieces.Add(piece);
                                state = ParseState.IdFound;
                            }
                            else if (state == ParseState.IdFound)
                            {
                                workingPieces.Add(piece);
                                state = ParseState.MessageFound;
                            }
                            else if (state == ParseState.MessageFound)
                            {
                                workingPieces[2] = workingPieces[2] + piece;
                            }
                        }
                    }
                }
                catch (Exception)
                {
                    Thread.Sleep(500);
                    continue;
                }
            }
        }

        private void ProcessMessage(string[] dataPieces)
        {
            string idMessage = dataPieces[1];
            string workingMessage = dataPieces[2];

            Invoke(new Action(() =>
            {
                listBox1.Items.Add(idMessage + "- " + workingMessage);
                if (checkBox1.Checked)
                    listBox1.SelectedIndex = listBox1.Items.Count - 1;
            }));
        }

        private void stop_Click(object sender, EventArgs e)
        {
            start.Enabled = true;
            stop.Enabled = false;
            textBox1.Enabled = true;
        }

        private void start_Click(object sender, EventArgs e)
        {
            start.Enabled = false;
            stop.Enabled = true;
            textBox1.Enabled = false;
            Settings.Default.LastIp = textBox1.Text;
            Settings.Default.Save();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            listBox1.Items.Clear();
        }
    }
}
