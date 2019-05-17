﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.Web.UI;

using Abstractspoon.Tdl.PluginHelpers;

namespace HTMLReportExporter
{
	public partial class HtmlReportTemplateForm : Form
	{
		private String m_TypeId;
		private Translator m_Trans;
		private TaskList m_Tasklist;

		private HtmlReportTemplate m_Template, m_PrevTemplate;
		private Timer m_TextChangeTimer;

		// --------------------------------------------------------------

		public HtmlReportTemplateForm(String typeId, Translator trans, TaskList tasks)
		{
			m_TypeId = typeId;
			m_Trans = trans;
			m_Tasklist = tasks;

			m_Template = new HtmlReportTemplate();
			m_PrevTemplate = new HtmlReportTemplate();
			m_TextChangeTimer = new Timer();

			InitializeComponentEx();
		}

		public HtmlReportTemplate ReportTemplate
		{
			get { return m_Template; }
		}

		private void InitializeComponentEx()
		{
			InitializeComponent();

			int bannerHeight = RhinoLicensing.CreateBanner(m_TypeId, this, m_Trans, 0/*20*/);

			this.Height = (this.Height + bannerHeight);
			this.Content.Location = new Point(0, bannerHeight);
			this.Content.Height = (this.Content.Height - bannerHeight);

			if (m_Template.Load("HtmlReportTemplate.txt"))
			{
				this.htmlReportHeaderControl.InnerHtml = m_Template.HeaderTemplate;
				this.htmlReportTitleControl.InnerHtml = m_Template.TitleTemplate;
				this.htmlReportTaskFormatControl.InnerHtml = m_Template.TaskTemplate;
				this.htmlReportFooterControl.InnerHtml = m_Template.FooterTemplate;
			}

			this.tabControl.SelectTab(taskPage);
			this.browserPreview.Navigate("about:blank");

			m_TextChangeTimer.Tick += new EventHandler(OnTextChangeTimer);
			m_TextChangeTimer.Interval = 500;
			m_TextChangeTimer.Start();
		}

		private void OnTextChangeTimer(object sender, EventArgs e)
		{
			if (!IsDisposed)
			{
				m_Template.HeaderTemplate = this.htmlReportHeaderControl.InnerHtml ?? "";
				m_Template.TitleTemplate = this.htmlReportTitleControl.InnerHtml ?? "";
				m_Template.TaskTemplate = this.htmlReportTaskFormatControl.InnerHtml ?? "";
				m_Template.FooterTemplate = this.htmlReportFooterControl.InnerHtml ?? "";

				if (!m_Template.Equals(m_PrevTemplate))
				{
					m_PrevTemplate.Copy(m_Template);

					try
					{
						using (var file = new System.IO.StreamWriter("HtmlReporterPreview.html"))
						{
							using (var html = new HtmlTextWriter(file))
							{
								var report = new HtmlReportBuilder(m_Tasklist, "HtmlReportTemplate.txt");

								report.BuildReport(html);
							}
						}

						browserPreview.Navigate("HtmlReporterPreview.html");
					}
					catch (Exception /*e*/)
					{
					}
				}
			}
		}

		protected override void OnClosing(CancelEventArgs e)
		{
			base.OnClosing(e);

			if (!e.Cancel)
			{
			}
		}

	}
}
