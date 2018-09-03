using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace ComAnalizer
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		SerialPortManager _spManager;

		public MainWindow()
		{
			InitializeComponent();
			Closing += (s, e) =>
			{
				_spManager.Dispose();
			};
			UserInitialization();
		}

		private void UserInitialization()
		{
			_spManager = new SerialPortManager();
			SerialSettings mySerialSettings = _spManager.CurrentSerialSettings;

			SettingsGB.DataContext = mySerialSettings;
			
			ParityCB.ItemsSource = Enum.GetValues(typeof(System.IO.Ports.Parity));
			StopBitsCB.ItemsSource = Enum.GetValues(typeof(System.IO.Ports.StopBits));

			_spManager.NewSerialDataRecieved += new EventHandler<SerialDataEventArgs>(_spManager_NewSerialDataRecieved);
		}

		void _spManager_NewSerialDataRecieved(object sender, SerialDataEventArgs e)
		{			
			
			// This application is connected to a GPS sending ASCCI characters, so data is converted to text
			string str = Encoding.ASCII.GetString(e.Data);
			DataTB.Items.Add(str);
			DataTB.ScrollIntoView(str);
		}

		// Handles the "Start Listening"-buttom click event
		private void btnStart_Click(object sender, EventArgs e)
		{
			_spManager.StartListening();
		}

		// Handles the "Stop Listening"-buttom click event
		private void btnStop_Click(object sender, EventArgs e)
		{
			_spManager.StopListening();
		}
	}
}
