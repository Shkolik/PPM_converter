using System;
using System.Collections.Generic;
using System.IO;
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
			Dispatcher.BeginInvoke(new Action(() =>
			{
				string str = Encoding.ASCII.GetString(e.Data);
				DataTB.AppendText(str);
				if (AutoScroll.IsChecked == true)
					DataTB.ScrollToEnd();
			}));
			// This application is connected to a GPS sending ASCCI characters, so data is converted to text

		}

		bool started;
		private void Button_Click(object sender, RoutedEventArgs e)
		{
			if (started)
			{
				_spManager.StopListening();
				started = false;
			}
			else
			{
				_spManager.StartListening();
				started = true;
			}

		}


		private void Clear_Click(object sender, RoutedEventArgs e)
		{
			_spManager.StopListening();
			started = false;

			DataTB.Text = null;
		}
		private void Save_Click(object sender, RoutedEventArgs e)
		{
			_spManager.StopListening();
			started = false;

			Microsoft.Win32.SaveFileDialog dlg = new Microsoft.Win32.SaveFileDialog();
			dlg.FileName = "Document"; // Default file name
			dlg.DefaultExt = ".txt"; // Default file extension
			dlg.Filter = "Text documents (.txt)|*.txt"; // Filter files by extension

			// Show save file dialog box
			var result = dlg.ShowDialog();

			// Process save file dialog box results
			if (result == true)
			{
				var newLines = new List<string>();
				var lines = DataTB.Text.Split(new[] { '\n' });
				foreach (var line in lines)
				{
					var values = line.Split(new[] { ' ' });
					var newLine = "";
					foreach (var value in values)
					{
						newLine += value + ", ";
					}
					newLines.Add(newLine.Trim(new[] { ' ', ',', '\r', '\n' }));					
				}
				// Save document
				File.WriteAllLines(dlg.FileName, newLines);
			}
		}
	}
}
