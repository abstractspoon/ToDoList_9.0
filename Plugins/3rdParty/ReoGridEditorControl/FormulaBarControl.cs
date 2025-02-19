﻿/*****************************************************************************
 * 
 * ReoGrid - .NET Spreadsheet Control
 * 
 * http://reogrid.net/
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * ReoGrid and ReoGridEditor is released under MIT license.
 *
 * Copyright (c) 2012-2016 Jing <lujing at unvell.com>
 * Copyright (c) 2012-2016 unvell.com, all rights reserved.
 * 
 ****************************************************************************/

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Windows.Forms.VisualStyles;

using unvell.Common;
using unvell.ReoGrid.Events;
using unvell.ReoGrid.Actions;

using IIControls;

namespace unvell.ReoGrid.Editor
{
	public partial class FormulaBarControl : UserControl
	{
		public bool FocusToGridAfterInputValue { get; set; }

		private int MinHeight = -1;
		private bool dragging = false;
		private int lastYDrag;

		public Color SplitterBackColor
		{
			set { this.splitterDown.BackColor = value; }
		}

		public FormulaBarControl()
		{
			InitializeComponent();

			MinHeight = (addressField.Height + (this.Height - txtFormula.Height));
			SplitterBackColor = Color.Empty;

			txtFormula.KeyDown += txtFormula_KeyDown;
			txtFormula.GotFocus += txtFormula_GotFocus;
			txtFormula.LostFocus += txtFormula_LostFocus;

			txtFormula.TextChanged += (s, e) =>
			{
				if (txtFormula.Text.Contains('\n'))
					txtFormula.ScrollBars = ScrollBars.Vertical;
				else
					txtFormula.ScrollBars = ScrollBars.None;
			};

			panel1.Paint += (s, e) =>
			{
				if (!VisualStyleRenderer.IsSupported)
					e.Graphics.DrawLine(SystemPens.ControlLight, panel1.Width - 1, 0, panel1.Width - 1, panel1.Height);
			};

			panel2.Paint += (s, e) =>
			{
				if (!VisualStyleRenderer.IsSupported)
					e.Graphics.DrawLine(SystemPens.ControlLight, 0, 0, 0, this.panel2.Bottom);
			};

			panel1.Resize += (s, e) =>
			{
				pictureBox1.Bounds = new Rectangle(
					panel1.ClientRectangle.Width - pictureBox1.Width - 1, 0,
					pictureBox1.Width, panel1.ClientRectangle.Height);
			};

			this.splitterDown.Paint += (s, e) =>
			{
				if (!VisualStyleRenderer.IsSupported)
				{
					e.Graphics.DrawLine(SystemPens.Control, 0, 0, splitterDown.Right, 0);
					e.Graphics.DrawLine(SystemPens.ControlDark, 0, splitterDown.Height - 1, splitterDown.Right, splitterDown.Height - 1);
				}
			};

			this.splitterDown.MouseDown += (s, e) =>
			{
				if (e.Button == System.Windows.Forms.MouseButtons.Left)
				{
					this.dragging = true;
					this.lastYDrag = this.PointToClient(Cursor.Position).Y;
				}
			};

			this.splitterDown.MouseMove += (s, e) =>
			{
				if (this.dragging)
				{
					int yDrag = this.PointToClient(Cursor.Position).Y;
					int height = this.Height + (yDrag - lastYDrag);

					if (height > 300)
					{
						height = 300;
						lastYDrag = height + splitterDown.Height / 2;
					}
					else if (height < MinHeight)
					{
						height = MinHeight;
						lastYDrag = height + splitterDown.Height / 2;
					}
					else
					{
						lastYDrag = yDrag;
					}

					this.Height = height;
				}
			};

			this.splitterDown.MouseUp += (s, e) =>
			{
				dragging = false;
			};

			this.leftPanel.Paint += (s, e) =>
			{
				if (!VisualStyleRenderer.IsSupported)
				{
					e.Graphics.DrawLine(SystemPens.ControlLight, 0, panel1.Height, this.leftPanel.Right, panel1.Height);
					e.Graphics.DrawLine(SystemPens.ControlLight, this.leftPanel.Right - 1, panel1.Height, this.leftPanel.Right - 1, this.leftPanel.Bottom);
				}
			};

			ToolStripEx.RemapSysColors(this.pictureBox1.Image as Bitmap);
		}

		private TextBox FocusedTextBox
		{
			get
			{
				if (txtFormula.Focused)
					return txtFormula;

				if (addressField.AddressBox.Focused)
					return addressField.AddressBox;

				return null;
			}
		}

		public bool EditControlCut()
		{
			var textBox = FocusedTextBox;

			textBox?.Cut();
			return (textBox != null);
		}

		public bool EditControlCopy()
		{
			var textBox = FocusedTextBox;

			textBox?.Copy();
			return (textBox != null);
		}

		public bool EditControlPaste()
		{
			var textBox = FocusedTextBox;

			textBox?.Paste();
			return (textBox != null);
		}

		public bool EditControlSelectAll()
		{
			var textBox = FocusedTextBox;

			textBox?.SelectAll();
			return (textBox != null);
		}

		public bool EditControlPaste(string text)
		{
			var textBox = FocusedTextBox;

			textBox?.Paste(text);
			return (textBox != null);
		}

		public new Color BackColor
		{
			get { return base.BackColor; }
			set
			{
				base.BackColor = value;

				this.leftPanel.BackColor = value;
				this.panel1.BackColor = value;
				this.panel2.BackColor = value;
				this.pictureBox1.BackColor = value;
			}
		}

		protected override void OnPaint(PaintEventArgs e)
		{
			base.OnPaint(e);

			e.Graphics.DrawLine(SystemPens.ControlDark, 0, 0, Right, 0);
		}

		private string backValue;

		void txtFormula_GotFocus(object sender, EventArgs e)
		{
			backValue = txtFormula.Text;
		}

		void txtFormula_LostFocus(object sender, EventArgs e)
		{
			ApplyNewFormula();
		}

		void txtFormula_KeyDown(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Enter)
			{
				if (e.Control)
				{
					txtFormula.SelectedText = "\r\n";
				}
				else
				{
					if (ApplyNewFormula())
						this.worksheet.MoveSelectionForward();

					if (FocusToGridAfterInputValue)
						grid.Focus();
				}

				e.Handled = true;
				e.SuppressKeyPress = true;
			}
		}

		private bool ApplyNewFormula()
		{
			if (this.worksheet != null)
			{
				var value = txtFormula.Text;

				if (value != backValue)
				{
					if (this.worksheet.IsEditing)
					{
						this.worksheet.EndEdit(value);
					}
					else
					{
						var pos = this.grid.CurrentWorksheet.FocusPos;
						
						var currentData = this.grid.CurrentWorksheet.GetCellData(pos);
						
						if (currentData != null || !string.IsNullOrEmpty(txtFormula.Text))
						{
							this.grid.DoAction(new SetCellDataAction(pos, txtFormula.Text));
						}

						return true;
					}
				}
			}
			return false;
		}

		private ReoGridControl grid;
		private Worksheet worksheet;

		public ReoGridControl GridControl
		{
			get
			{
				return grid;
			}
			set
			{
				if (addressField != null)
				{
					addressField.GridControl = value;
				}
				
				if (grid != null)
				{
					grid.Disposed -= grid_Disposed;

					grid.CurrentWorksheetChanged -= grid_CurrentWorksheetChanged;
					this.worksheet = null;
					this.txtFormula.Text = string.Empty;
				}

				grid = value;

				if (grid != null)
				{
					grid.Disposed += grid_Disposed;

					this.worksheet = grid.CurrentWorksheet;
					grid.CurrentWorksheetChanged += grid_CurrentWorksheetChanged;
				}
			}
		}

		void grid_CurrentWorksheetChanged(object sender, EventArgs e)
		{
			if (this.worksheet != null)
			{
				this.worksheet.FocusPosChanged -= grid_FocusPosChanged;
			}

			this.worksheet = grid.CurrentWorksheet;

			if (this.worksheet != null)
			{
				ReadFormulaFromCell();
				this.worksheet.FocusPosChanged += grid_FocusPosChanged;
			}
		}

		void grid_FocusPosChanged(object sender, CellPosEventArgs e)
		{
			ReadFormulaFromCell();
		}

		private void ReadFormulaFromCell()
		{
			if (this.worksheet == null)
			{
				txtFormula.Text = string.Empty;
			}
			else
			{
				var cell = this.worksheet.GetCell(this.worksheet.FocusPos);

				if (cell == null)
				{
					txtFormula.Text = string.Empty;
				}
				else
				{
					var formula = cell.Formula;

					if (!string.IsNullOrEmpty(formula))
					{
						txtFormula.Text = "=" + formula;
					}
					else
					{
						txtFormula.Text = Convert.ToString(cell.Data);
					}
				}
			}
		}

		void grid_Disposed(object sender, EventArgs e)
		{
			this.GridControl = null;
		}

		private void formulaTextboxPanel_Click(object sender, EventArgs e)
		{
			txtFormula.Focus();
		}

		public void RefreshCurrentAddress()
		{
			if (this.addressField != null)
			{
				this.addressField.RefreshCurrentAddress();
			}
		}

		public void RefreshCurrentFormula()
		{
			ReadFormulaFromCell();
		}
	}

}
