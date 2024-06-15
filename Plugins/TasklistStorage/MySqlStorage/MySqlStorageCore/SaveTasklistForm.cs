﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using MySql.Data.MySqlClient;

namespace MySqlStorage
{
	public partial class SaveTasklistForm : Form
	{
		public SaveTasklistForm(MySqlConnection conn, ConnectionDefinition def)
		{
			InitializeComponent();

			m_Database.Text = string.Format("{0}/{1}", def.Server, def.Database);
			m_Tasklist.Text = def.TasklistName;

			m_Tasklists.Initialise(conn, false);
		}

		public TasklistInfo TasklistInfo
		{
			get
			{
				var tasklist = (m_Tasklists.SelectedItem as TasklistInfo);

				if (tasklist != null)
				{
					if (m_Tasklist.Text.Equals(tasklist.Name, StringComparison.InvariantCultureIgnoreCase))
						return tasklist;
				}

				// else
				return new TasklistInfo() { Name = m_Tasklist.Text };
			}
		}
	}
}
