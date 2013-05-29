using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace WinHookApp
{
    public partial class SlimHook : Form
    {
        public SlimHook()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            HookProcess(Convert.ToInt32(txtPID.Text));
        }

        [DllImport("inject.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
        internal static extern int Inject(int pid, StringBuilder amsBase);
        public void HookProcess(int pid)
        {
            string exe = System.Reflection.Assembly.GetExecutingAssembly().Location;
            string asmPath = System.IO.Path.GetDirectoryName(exe);
            Inject(pid, new StringBuilder(asmPath));
        }
    }
}
