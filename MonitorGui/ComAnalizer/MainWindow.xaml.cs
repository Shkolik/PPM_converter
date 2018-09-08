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
				MinDataTB.Text = GetMins(GetIntValues(), 6);
				MaxDataTB.Text = GetMax(GetIntValues(), 6);
				started = false;
			}
			else
			{
				_spManager.StartListening();
				MinDataTB.Text = "";
				MaxDataTB.Text = "";
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
				// Save document
				File.WriteAllLines(dlg.FileName, GetFileData());
			}
		}

		private List<string> GetFileData()
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

			return newLines;
		}
		private string GetMins(List<List<int>> values, int columns)
		{
			var res = "";
			for (var i = 0; i < columns; i++)
			{
				try
				{
					res += values.Where(v => v.Count >= columns &&  v[i] > 999).Min(v => v[i]) + " ";
				}
				catch (Exception ex)
				{
				}

			}
			return res;
		}

		private string GetMax(List<List<int>> values, int columns)
		{
			var res = "";
			for (var i = 0; i < columns; i++)
			{
				try
				{
					res += values.Where(v => v.Count >= columns).Max(v => v[i]) + " ";
				}
				catch (Exception ex)
				{
				}
			}
			return res;
		}

		private List<List<int>> GetIntValues()
		{
			var res = new List<List<int>>();

			var lines = DataTB.Text.Split(new[] { '\n' });
			foreach (var line in lines)
			{
				var values = line.Split(new[] { ' ' });
				var intValues = new List<int>();
				foreach (var value in values)
				{
					if (int.TryParse(value, out int intVal))
						intValues.Add(intVal);
					else
						intValues.Add(0);
				}
				res.Add(intValues);
			}
			return res;
		}
	}
}
