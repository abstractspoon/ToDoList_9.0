using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using System.Diagnostics;

using Abstractspoon.Tdl.PluginHelpers;
using Abstractspoon.Tdl.PluginHelpers.ColorUtil;

namespace DayViewUIExtension
{

	public class TDLDayView : Calendar.DayView, ILabelTipHandler
    {
        private UInt32 m_SelectedTaskID = 0;
		private UInt32 m_VisibleSelectedTaskID = 0;
		private UInt32 m_MaxTaskID = 0;

		private int m_UserMinSlotHeight = -1;

        private Boolean m_HideParentTasks = true;
		private Boolean m_DisplayTasksContinuous = true;
		private Boolean m_HideTasksWithoutTimes = true;
        private Boolean m_HideTasksSpanningWeekends = false;
        private Boolean m_HideTasksSpanningDays = false;
		private Boolean m_ShowFutureOcurrences = true;

		private Dictionary<UInt32, TaskItem> m_Items;
        private Dictionary<UInt32, CalendarExtensionItem> m_ExtensionItems;
		private List<CustomAttributeDefinition> m_CustomDates;

		private TDLRenderer m_Renderer;
		private LabelTip m_LabelTip;
		private UIExtension.TaskRecurrences m_TaskRecurrences;

		private int LabelTipBorder
		{
			get { return DPIScaling.Scale(4); }
		}

        public Color GridlineColor
        {
            set
            {
                if (value != m_Renderer.GridlineColor)
                {
                    m_Renderer.GridlineColor = value;
                    Invalidate();
                }
            }
        }

		// ----------------------------------------------------------------

		public Boolean ReadOnly { get; set; }

        public TDLDayView(UIExtension.TaskIcon taskIcons, UIExtension.TaskRecurrences taskRecurrences, int minSlotHeight)
        {
            minHourLabelWidth = DPIScaling.Scale(minHourLabelWidth);
            hourLabelIndent = DPIScaling.Scale(hourLabelIndent);
            dayHeadersHeight = DPIScaling.Scale(dayHeadersHeight);
            longAppointmentSpacing = DPIScaling.Scale(longAppointmentSpacing);
			dayGripWidth = 1; // to match app styling

            m_Renderer = new TDLRenderer(Handle, taskIcons);
			m_UserMinSlotHeight = minSlotHeight;
            m_LabelTip = new LabelTip(this);
			m_TaskRecurrences = taskRecurrences;

			m_Items = new Dictionary<UInt32, TaskItem>();
			m_ExtensionItems = new Dictionary<uint, CalendarExtensionItem>();
			m_CustomDates = new List<CustomAttributeDefinition>();

			InitializeComponent();
        }

        // ILabelTipHandler implementation
        public Font GetFont()
        {
            return m_Renderer.BaseFont;
        }

        public Control GetOwner()
        {
            return this;
        }

        public UInt32 ToolHitTest(Point ptScreen, ref String tipText, ref Rectangle toolRect, ref bool multiLine)
        {
			if (IsResizingAppointment())
				return 0;

            var pt = PointToClient(ptScreen);
            Calendar.Appointment appt = GetAppointmentAt(pt.X, pt.Y);

            if (appt == null)
                return 0;

            var taskItem = appt as TaskItem;

            if ((taskItem == null) || !taskItem.TextRect.Contains(pt))
                return 0;

			toolRect = taskItem.TextRect;
			toolRect.Inflate(m_Renderer.TextPadding, m_Renderer.TextPadding);

			if (IsLongAppt(appt))
			{
				// single line tooltips
				if (m_LabelTip.CalcTipHeight(taskItem.Title, toolRect.Width) <= toolRect.Height)
					return 0;

				multiLine = false; // always
			}
			else
			{
				var availRect = GetTrueRectangle();

				if (taskItem.TextRect.Top < availRect.Top)
				{
					// If the top of the text rectangle is hidden we always 
					// need a label tip so we just clip to the avail space
					toolRect.Intersect(availRect);
				}
				else
				{
					// Determine if text will fit in what's visible of the task
					toolRect.Intersect(availRect);

					if (m_LabelTip.CalcTipHeight(taskItem.Title, toolRect.Width) < toolRect.Height)
						return 0;
				}

				multiLine = true; // always
			}

			tipText = taskItem.Title;

            return taskItem.Id;
        }

        protected override void WndProc(ref Message m)
        {
            if (m_LabelTip!= null)
                m_LabelTip.ProcessMessage(m);

            base.WndProc(ref m);
        }

		protected override Calendar.SelectionTool NewSelectionTool()
		{
			return new TDLSelectionTool();
		}

        protected void InitializeComponent()
        {
            Calendar.DrawTool drawTool = new Calendar.DrawTool();
            drawTool.DayView = this;

            this.ActiveTool = drawTool;
            this.AllowInplaceEditing = true;
            this.AllowNew = false;
            this.AmPmDisplay = true;
            this.Anchor = (System.Windows.Forms.AnchorStyles.Bottom |
                                     System.Windows.Forms.AnchorStyles.Left |
                                     System.Windows.Forms.AnchorStyles.Right);
            this.AppHeightMode = Calendar.DayView.AppHeightDrawMode.TrueHeightAll;
            this.DrawAllAppBorder = false;
            this.Location = new System.Drawing.Point(0, 0);
            this.MinHalfHourApp = false;
            this.Name = "m_dayView";
            this.Renderer = m_Renderer;
            this.SelectionEnd = new System.DateTime(((long)(0)));
            this.SelectionStart = new System.DateTime(((long)(0)));
            this.Size = new System.Drawing.Size(798, 328);
            this.SlotsPerHour = 4;
            this.TabIndex = 0;
            this.Text = "m_dayView";
            this.ReadOnly = false;

			this.ResolveAppointments += new Calendar.ResolveAppointmentsEventHandler(this.OnResolveAppointments);
            this.SelectionChanged += new Calendar.AppointmentEventHandler(this.OnSelectionChanged);
            this.WeekChange += new Calendar.WeekChangeEventHandler(OnWeekChanged);

		}

		public bool ShowLabelTips
		{
			set { m_LabelTip.Active = value; }
			get { return m_LabelTip.Active; }
		}

        public bool IsTaskDisplayable(UInt32 dwTaskID)
        {
            if (dwTaskID == 0)
                return false;

			CalendarExtensionItem extItem;

			if (m_ExtensionItems.TryGetValue(dwTaskID, out extItem))
				dwTaskID = extItem.RealTaskId;
			
			TaskItem item;

			if (m_Items.TryGetValue(dwTaskID, out item))
	            return IsItemDisplayable(item);

			// else
			return false;
        }

        public Boolean HideParentTasks
        {
            get { return m_HideParentTasks; }
            set
            {
                if (value != m_HideParentTasks)
                {
                    m_HideParentTasks = value;
                    FixupSelection(false, true);
                }
            }
        }

		public Boolean ShowFutureOccurrences
		{
			get { return m_ShowFutureOcurrences; }
			set
			{
				if (value != m_ShowFutureOcurrences)
				{
					m_ShowFutureOcurrences = value;
					Invalidate();
				}
			}
		}

		public Boolean AutoCalculateDependencyDates
		{
			get; set;
		}

		public Boolean DisplayTasksContinuous
		{
			get { return m_DisplayTasksContinuous; }
			set
			{
				if (value != m_DisplayTasksContinuous)
				{
					m_DisplayTasksContinuous = value;
					FixupSelection(false, true);
				}
			}
		}

		public Boolean HideTasksWithoutTimes
        {
            get { return m_HideTasksWithoutTimes; }
            set
            {
                if (value != m_HideTasksWithoutTimes)
                {
                    m_HideTasksWithoutTimes = value;
                    FixupSelection(false, true);
                }
            }
        }

        public Boolean HideTasksSpanningWeekends
        {
            get { return m_HideTasksSpanningWeekends; }
            set
            {
                if (value != m_HideTasksSpanningWeekends)
                {
                    m_HideTasksSpanningWeekends = value;
                    FixupSelection(false, true);
                }
            }
        }

        public Boolean HideTasksSpanningDays
        {
            get { return m_HideTasksSpanningDays; }
            set
            {
                if (value != m_HideTasksSpanningDays)
                {
                    m_HideTasksSpanningDays = value;
                    FixupSelection(false, true);
                }
            }
        }

        public UInt32 GetSelectedTaskID()
        {
            if (!IsTaskDisplayable(m_SelectedTaskID))
                return 0;

			// If an extension item is selected, return the 'real' task Id
			CalendarExtensionItem extItem;

			if (m_ExtensionItems.TryGetValue(m_SelectedTaskID, out extItem))
				return extItem.RealTaskId;

			// else
			return m_SelectedTaskID;
        }

		public bool GetSelectedTaskDates(out DateTime from, out DateTime to)
		{
			from = to = Calendar.Appointment.NullDate;

			UInt32 selTaskID = GetSelectedTaskID();

			if (selTaskID == 0)
				return false;

			TaskItem item;

			if (!m_Items.TryGetValue(selTaskID, out item))
				return false;

			if (!item.HasValidDates())
				return false;

			from = item.StartDate;
			to = item.EndDate;

			return true;
		}

        public bool GetTask(UIExtension.GetTask getTask, ref UInt32 taskID)
        {
            switch (getTask)
            {
            case UIExtension.GetTask.GetNextTask:
                // TODO
                break;

            case UIExtension.GetTask.GetPrevTask:
                // TODO
                break;

            case UIExtension.GetTask.GetNextVisibleTask:
                // TODO
                break;

            case UIExtension.GetTask.GetNextTopLevelTask:
                // TODO
                break;

            case UIExtension.GetTask.GetPrevVisibleTask:
                // TODO
                break;

            case UIExtension.GetTask.GetPrevTopLevelTask:
                // TODO
                break;
            }

            // all else
            return false;
        }

        public bool SelectTask(String text, UIExtension.SelectTask selectTask, bool caseSensitive, bool wholeWord, bool findReplace)
        {
            if (text == String.Empty)
                return false;

            switch (selectTask)
            {
            case UIExtension.SelectTask.SelectFirstTask:
                // TODO
                break;

            case UIExtension.SelectTask.SelectNextTask:
                // TODO
                break;

            case UIExtension.SelectTask.SelectNextTaskInclCurrent:
                // TODO
                break;

            case UIExtension.SelectTask.SelectPrevTask:
                // TODO
                break;

            case UIExtension.SelectTask.SelectLastTask:
                // TODO
                break;
            }

            // all else
            return false;
        }

        public Calendar.Appointment FixupSelection(bool scrollToTask, bool allowNotify)
        {
			// Our base class clears the selected appointment whenever
			// the week changes so we can't always rely on 'SelectedAppointmentId'
            UInt32 prevSelTaskID = m_VisibleSelectedTaskID;
            UInt32 selTaskID = GetSelectedTaskID();

			m_VisibleSelectedTaskID = selTaskID;
			
			if (selTaskID > 0)
            {
                TaskItem item;

                if (m_Items.TryGetValue(selTaskID, out item))
                {
                    if (scrollToTask)
                    {
                        if (item.StartDate != Calendar.Appointment.NullDate)
                        {
                            if (!IsItemWithinRange(item, StartDate, EndDate))
                                StartDate = item.StartDate;

                            SelectedAppointment = item;
                        }
                    }
                    else if (IsItemWithinRange(item, StartDate, EndDate))
                    {
                        SelectedAppointment = item;
                    }
                }
				else
				{
					SelectedAppointment = null;
				}
			}
			else
			{
				SelectedAppointment = null;
			}
			
			// Notify parent of changes
			if (allowNotify && (GetSelectedTaskID() != prevSelTaskID))
			{
				TaskItem item = null;
				m_Items.TryGetValue(m_VisibleSelectedTaskID, out item);

				RaiseSelectionChanged(item);
			}

			return SelectedAppointment;
		}

		public bool SelectTask(UInt32 dwTaskID)
		{
            m_SelectedTaskID = dwTaskID;
            FixupSelection(true, false);

			return (GetSelectedTaskID() != 0);
		}

        public void GoToToday()
        {
            StartDate = DateTime.Now;

			// And scroll vertically to first short task
			var appointments = GetMatchingAppointments(StartDate, EndDate, true);

			if (appointments != null)
			{
				foreach (var appt in appointments)
				{
					if (!IsLongAppt(appt) && EnsureVisible(appt, false))
						break;
				}
			}
			else
			{
				ScrollToTop();
			}

			Invalidate();
        }

		public UIExtension.HitResult HitTest(Int32 xScreen, Int32 yScreen)
		{
			System.Drawing.Point pt = PointToClient(new System.Drawing.Point(xScreen, yScreen));
			Calendar.Appointment appt = GetAppointmentAt(pt.X, pt.Y);

			if (appt != null)
			{
				return UIExtension.HitResult.Task;
			}
			else if (GetTrueRectangle().Contains(pt))
			{
				return UIExtension.HitResult.Tasklist;
			}

			// else
			return UIExtension.HitResult.Nowhere;
		}

		public UInt32 HitTestTask(Int32 xScreen, Int32 yScreen)
		{
			System.Drawing.Point pt = PointToClient(new System.Drawing.Point(xScreen, yScreen));
			Calendar.Appointment appt = GetAppointmentAt(pt.X, pt.Y);

			if (appt != null)
			{
				if (appt is CalendarExtensionItem)
					return (appt as CalendarExtensionItem).RealTaskId;

				return appt.Id;
			}

			return 0;
		}

		public Calendar.Appointment GetRealAppointmentAt(int x, int y)
		{
			return GetRealAppointment(GetAppointmentAt(x, y));
		}

		public Calendar.Appointment GetAppointment(UInt32 taskID)
		{
			CalendarExtensionItem extItem;

			if (m_ExtensionItems.TryGetValue(taskID, out extItem))
				return extItem;

			TaskItem item;

			if (m_Items.TryGetValue(taskID, out item))
				return item;

			return null;
		}

		public Calendar.Appointment GetRealAppointment(Calendar.Appointment appt)
		{
			if ((appt != null) && (appt is CalendarExtensionItem))
				return (appt as CalendarExtensionItem).RealTask;

			return appt;
		}

		public bool GetSelectedItemLabelRect(ref Rectangle rect)
		{
			FixupSelection(true, false);
			var appt = GetRealAppointment(SelectedAppointment);

			EnsureVisible(appt, false);
			Update(); // make sure draw rects are updated

			if (GetAppointmentRect(appt, ref rect))
			{
				TaskItem item = (appt as TaskItem);
				bool hasIcon = m_Renderer.TaskHasIcon(item);

				if (IsLongAppt(appt))
				{
					// Gripper
					if (appt.StartDate >= StartDate)
						rect.X += 8;
					else
						rect.X -= 3;

					if (hasIcon)
						rect.X += 16;

					rect.X += 1;
					rect.Height += 1;
				}
				else
				{
					if (hasIcon)
					{
						rect.X += 18;
					}
					else
					{
						// Gripper
						rect.X += 8;
					}

					rect.X += 1;
					rect.Y += 1;

					rect.Height = (GetFontHeight() + 4); // 4 = border
				}

				return true;
			}

			return false;
		}

		public bool IsItemDisplayable(TaskItem item)
		{
			// Always show a task if it is currently being dragged
			if (IsResizingAppointment() && (item == SelectedAppointment))
				return true;

			if (HideParentTasks && item.IsParent)
				return false;

			if (!item.HasValidDates())
				return false;

			if (HideTasksSpanningWeekends)
			{
				if (DateUtil.WeekOfYear(item.StartDate) != DateUtil.WeekOfYear(item.EndDate))
					return false;
			}

            if (HideTasksSpanningDays)
            {
                if (item.StartDate.Date != item.EndDate.Date)
                    return false;
            }

			if (HideTasksWithoutTimes)
			{
				if (TaskItem.IsStartOfDay(item.StartDate) && TaskItem.IsEndOfDay(item.EndDate))
					return false;
			}

			return true;
		}

		private bool IsItemWithinRange(Calendar.Appointment appt, DateTime startDate, DateTime endDate)
		{
			// sanity check
			if (!appt.HasValidDates())
				return false;

			// Task must at least intersect the range
			if ((appt.StartDate >= endDate) || (appt.EndDate <= startDate))
				return false;

			if (!DisplayTasksContinuous)
			{
				if ((appt.StartDate < startDate) && (appt.EndDate > endDate))
					return false;
			}

            return true;
		}

		public void UpdateTasks(TaskList tasks,	UIExtension.UpdateType type)
		{
            switch (type)
			{
				case UIExtension.UpdateType.Delete:
				case UIExtension.UpdateType.All:
					// Rebuild
					m_Items.Clear();
					m_MaxTaskID = 0;
					SelectedAppointment = null;
					break;

				case UIExtension.UpdateType.New:
				case UIExtension.UpdateType.Edit:
					// In-place update
					break;
			}

			// Update custom attribute definitions
			m_CustomDates = tasks.GetCustomAttributes(CustomAttributeDefinition.Attribute.Date);

			// Update the tasks
			Task task = tasks.GetFirstTask();

			while (task.IsValid() && ProcessTaskUpdate(task, type))
				task = task.GetNextTask();

			// Scroll to the selected item if it was modified and is 'visible'
			if (tasks.HasTask(m_SelectedTaskID) && IsTaskDisplayable(m_SelectedTaskID))
                EnsureVisible(SelectedAppointment, true);

			SelectionStart = SelectionEnd;

            AdjustVScrollbar();
            Invalidate();
        }

		private bool ProcessTaskUpdate(Task task, UIExtension.UpdateType type)
		{
			if (!task.IsValid())
				return false;

			TaskItem item;
			UInt32 taskID = task.GetID();

			m_MaxTaskID = Math.Max(m_MaxTaskID, taskID); // needed for extension occurrences

			// Built-in of attributes
			if (m_Items.TryGetValue(taskID, out item))
			{
				item.UpdateTaskAttributes(task, m_CustomDates, type, false);
			}
			else
			{
				item = new TaskItem();
				item.UpdateTaskAttributes(task, m_CustomDates, type, true);
			}

			m_Items[taskID] = item;

			// Process children
			Task subtask = task.GetFirstSubtask();

			while (subtask.IsValid() && ProcessTaskUpdate(subtask, type))
				subtask = subtask.GetNextTask();

			return true;
		}

		public Boolean StrikeThruDoneTasks
		{
			get { return m_Renderer.StrikeThruDoneTasks; }
			set
			{
                if (m_Renderer.StrikeThruDoneTasks != value)
				{
                    m_Renderer.StrikeThruDoneTasks = value;
					Invalidate();
				}
			}
		}

        public Boolean TaskColorIsBackground
        {
            get { return m_Renderer.TaskColorIsBackground; }
            set
            {
                if (m_Renderer.TaskColorIsBackground != value)
                {
                    m_Renderer.TaskColorIsBackground = value;
                    Invalidate();
                }
            }
        }

		public Boolean ShowParentsAsFolder
		{
			get { return m_Renderer.ShowParentsAsFolder; }
			set
			{
				if (m_Renderer.ShowParentsAsFolder != value)
				{
					m_Renderer.ShowParentsAsFolder = value;
					Invalidate();
				}
			}
		}

        public void SetFont(String fontName, int fontSize)
        {
            m_Renderer.SetFont(fontName, fontSize);

            LongAppointmentHeight = Math.Max(m_Renderer.BaseFont.Height + 4, 17);
        }
        
        public int GetFontHeight()
        {
            return m_Renderer.GetFontHeight();
        }

   		public void SetUITheme(UITheme theme)
		{
            m_Renderer.Theme = theme;
            Invalidate(true);
		}

		public override DateTime GetDateAt(int x, bool longAppt)
		{
			DateTime date = base.GetDateAt(x, longAppt);

			if (longAppt && (date >= EndDate))
			{
				date = EndDate.AddSeconds(-1);
			}

			return date;
		}

        public override TimeSpan GetTimeAt(int y)
        {
            TimeSpan time = base.GetTimeAt(y);
            
            if (time == new TimeSpan(1, 0, 0, 0))
                time = time.Subtract(new TimeSpan(0, 0, 1));

            return time;
        }

		protected override void DrawDay(PaintEventArgs e, Rectangle rect, DateTime time)
		{
			e.Graphics.FillRectangle(SystemBrushes.Window, rect);

			if (SystemInformation.HighContrast)
			{
				// Draw selection first because it's opaque
				DrawDaySelection(e, rect, time);

				DrawDaySlotSeparators(e, rect, time);
				DrawNonWorkHours(e, rect, time);
				DrawToday(e, rect, time);
				DrawDayAppointments(e, rect, time);
			}
			else
			{
				DrawDaySlotSeparators(e, rect, time);
				DrawNonWorkHours(e, rect, time);
				DrawToday(e, rect, time);
				DrawDayAppointments(e, rect, time);

				// Draw selection last because it's translucent
				DrawDaySelection(e, rect, time);
			}

			DrawDayGripper(e, rect);
		}

		private bool WantDrawToday(DateTime time)
		{
			if (!m_Renderer.Theme.HasAppColor(UITheme.AppColor.Today))
				return false;
			
			return (time.Date == DateTime.Now.Date);
		}

		protected void DrawToday(PaintEventArgs e, Rectangle rect, DateTime time)
		{
			if (!WantDrawToday(time))
				return;

			using (var brush = new SolidBrush(m_Renderer.Theme.GetAppDrawingColor(UITheme.AppColor.Today, 128)))
				e.Graphics.FillRectangle(brush, rect);
		}

		protected void DrawNonWorkHours(PaintEventArgs e, Rectangle rect, DateTime time)
		{
			if (m_Renderer.Theme.HasAppColor(UITheme.AppColor.Weekends) && WeekendDays.Contains(time.DayOfWeek))
			{
				var weekendColor = m_Renderer.Theme.GetAppDrawingColor(UITheme.AppColor.Weekends, 128);

				// If this is also 'today' then convert to gray so it doesn't 
				// impose too much when the today colour is laid on top
				if (WantDrawToday(time))
					weekendColor = DrawingColor.ToGray(weekendColor);

				using (var brush = new SolidBrush(weekendColor))
					e.Graphics.FillRectangle(brush, rect);
			}
			else if (m_Renderer.Theme.HasAppColor(UITheme.AppColor.NonWorkingHours))
			{
				var nonWorkColor = m_Renderer.Theme.GetAppDrawingColor(UITheme.AppColor.NonWorkingHours, 128);

				// If this is also 'today' then convert to gray so it doesn't 
				// impose too much when the today colour is laid on top
				if (WantDrawToday(time))
					nonWorkColor = DrawingColor.ToGray(nonWorkColor);

				using (SolidBrush brush = new SolidBrush(nonWorkColor))
				{
					DrawNonWorkHours(e, new HourMin(0, 0), WorkStart, rect, brush);
					DrawNonWorkHours(e, LunchStart, LunchEnd, rect, brush);
					DrawNonWorkHours(e, WorkEnd, new HourMin(24, 0), rect, brush);
				}
			}
		}
		
		protected void DrawNonWorkHours(PaintEventArgs e, HourMin start, HourMin end, Rectangle rect, Brush brush)
		{
			if (start < end)
			{
				Rectangle hoursRect = GetHourRangeRectangle(start, end, rect);

				if (hoursRect.Y < this.HeaderHeight)
				{
					hoursRect.Height -= this.HeaderHeight - hoursRect.Y;
					hoursRect.Y = this.HeaderHeight;
				}

				e.Graphics.FillRectangle(brush, hoursRect);
			}
		}

		public bool EnsureSelectionVisible(bool partialOK)
		{
			var appt = FixupSelection(true, false);

			if (appt == null)
				return false;

			return EnsureVisible(appt, partialOK);
		}

		public override bool EnsureVisible(Calendar.Appointment appt, bool partialOK)
		{
			if ((appt == null) && (m_SelectedTaskID != 0))
			{
				TaskItem item;

				if (m_Items.TryGetValue(m_SelectedTaskID, out item))
					appt = item;
			}

			return base.EnsureVisible(appt, partialOK);
		}

		bool WantDrawAppointmentSelected(Calendar.Appointment appt)
		{
			if (m_SelectedTaskID == appt.Id)
				return true;

			// Show real task as selected when a future item 
			// is selected and vice versa
			var selAppt = GetAppointment(m_SelectedTaskID);

			if (!(selAppt is CustomDateAttribute))
			{
				var selRealID = GetSelectedTaskID();

				if (appt is FutureOccurrence)
					return (selRealID == (appt as FutureOccurrence).RealTaskId);

				if (selAppt is FutureOccurrence)
					return (selRealID == appt.Id);
			}

			return false;
		}

		protected override void DrawAppointment(Graphics g, Rectangle rect, Calendar.Appointment appointment, bool isSelected, Rectangle gripRect)
		{
			isSelected = WantDrawAppointmentSelected(appointment);
			
			// Our custom gripper bar
			gripRect = rect;
			gripRect.Inflate(-2, -2);
			gripRect.Width = 5;

            // If the start date precedes the start of the week then extend the
            // draw rect to the left so the edge is clipped and likewise for the right.
            bool longAppt = IsLongAppt(appointment);

            if (longAppt)
            {
                if (appointment.StartDate < StartDate)
                {
                    rect.X -= 4;
                    rect.Width += 4;

                    gripRect.X = rect.X;
                    gripRect.Width = 0;
                }
                else if (appointment.StartDate > StartDate)
                {
                    rect.X++;
                    rect.Width--;

                    gripRect.X++;
                }

                if (appointment.EndDate >= EndDate)
                {
                    rect.Width += 5;
                }
            }
            else // day appointment
            {
                if (appointment.StartDate.TimeOfDay.TotalHours == 0.0)
                {
                    rect.Y++;
                    rect.Height--;
                }

                rect.Width -= 1;
            }
			
			m_Renderer.DrawAppointment(g, rect, appointment, longAppt, isSelected, gripRect);
		}

		private void OnResolveAppointments(object sender, Calendar.ResolveAppointmentsEventArgs args)
		{
			args.Appointments = GetMatchingAppointments(args.StartDate, args.EndDate);
		}

		private List<Calendar.Appointment> GetMatchingAppointments(DateTime start, DateTime end, bool sorted = false)
		{
			// Extension items are always populated on demand
			m_ExtensionItems.Clear();

			var appts = new List<Calendar.Appointment>();
			UInt32 nextExtId = (((m_MaxTaskID / 1000) + 1) * 1000);

			foreach (var pair in m_Items)
			{
				TaskItem item = pair.Value;

				if (!IsItemDisplayable(item))
					continue;

				if (IsItemWithinRange(item, start, end))
					appts.Add(item);

				if (m_ShowFutureOcurrences && item.IsRecurring)
				{
					// Add this task's future items for the current date range
					// Note: we deliberately double the range else we lose 
					// future items which overlap the the current item
					var futureItems = m_TaskRecurrences.Get(item.Id, StartDate, EndDate.AddDays(DaysShowing));

					if (futureItems != null)
					{
						foreach (var futureItem in futureItems)
						{
							var futureAppt = new FutureOccurrence(item, nextExtId, futureItem.Item1, futureItem.Item2);

							if (IsItemWithinRange(futureAppt, start, end))
							{
								m_ExtensionItems[nextExtId++] = futureAppt;
								appts.Add(futureAppt);
							}
						}
					}
				}

				if (m_CustomDates.Count > 0)
				{
					foreach (var attrib in m_CustomDates)
					{
						DateTime date;

						if (item.CustomDates.TryGetValue(attrib.Id, out date))
						{
							var customDate = new CustomDateAttribute(item, nextExtId, attrib.Id, date);

							if (IsItemWithinRange(customDate, start, end))
							{
								m_ExtensionItems[nextExtId++] = customDate;
								appts.Add(customDate);
							}
						}
					}
				}
			}

			if (sorted)
				appts.Sort((a, b) => (int)(b.StartDate.Ticks - a.StartDate.Ticks));

			return appts;
		}

		private void OnSelectionChanged(object sender, Calendar.AppointmentEventArgs args)
        {
            if (args.Appointment != null)
			{
				m_SelectedTaskID = args.Appointment.Id;

				// User made this selection so the task must be visible
				m_VisibleSelectedTaskID = m_SelectedTaskID;
			}
		}

        private void OnWeekChanged(object sender, Calendar.WeekChangeEventArgs args)
        {
            FixupSelection(false, true);
        }

        protected override void OnGotFocus(EventArgs e)
        {
            base.OnGotFocus(e);

            Invalidate();
            Update();
        }

        protected override void OnLostFocus(EventArgs e)
        {
            base.OnLostFocus(e);

            Invalidate();
            Update();
        }

		protected override void OnMouseDown(MouseEventArgs e)
		{
			// let the base class initiate resizing if it wants
			base.OnMouseDown(e);

			// Cancel resizing if our task is not editable
			if (IsResizingAppointment())
			{
				var mode = GetMode(SelectedAppointment, e.Location);

				if (!CanModifyAppointmentDates(SelectedAppointment, mode))
				{
					CancelAppointmentResizing();
				}
			}
		}

		private Calendar.SelectionTool.Mode GetMode(Calendar.Appointment appt, Point mousePos)
		{
			if (ReadOnly || (appt == null))
				return Calendar.SelectionTool.Mode.None;

			var selTool = (ActiveTool as Calendar.SelectionTool);

			if (selTool == null)
			{
				selTool = new Calendar.SelectionTool();
				selTool.DayView = this;
			}

			return selTool.GetMode(mousePos, appt);
		}

		public new bool CancelAppointmentResizing()
		{
			if (base.CancelAppointmentResizing())
			{
				var taskItem = (SelectedAppointment as TaskItem);

				if (taskItem != null)
				{
					taskItem.RestoreOriginalDates();
				}
				else if (SelectedAppointment is CustomDateAttribute)
				{
					var custDate = (SelectedAppointment as CustomDateAttribute);
					custDate.RealTask.CustomDates[custDate.AttributeId] = custDate.OriginalDate;
				}

				Invalidate();

				return true;
			}

			return false;
		}

		private bool CanModifyAppointmentDates(Calendar.Appointment appt, Calendar.SelectionTool.Mode mode)
		{
			if (appt.Locked)
				return false;

			// Disable start date editing for tasks with dependencies that are auto-calculated
			// Disable resizing for custom date attributes
			bool isCustomDate = (appt is CustomDateAttribute);
			bool hasDepends = ((appt is TaskItem) && (appt as TaskItem).HasDependencies);

			switch (mode)
			{
			case Calendar.SelectionTool.Mode.Move:
				return (!hasDepends || !AutoCalculateDependencyDates);

			case Calendar.SelectionTool.Mode.ResizeTop:
			case Calendar.SelectionTool.Mode.ResizeLeft:
				return ((!hasDepends || !AutoCalculateDependencyDates) && !isCustomDate);

			case Calendar.SelectionTool.Mode.ResizeBottom:
			case Calendar.SelectionTool.Mode.ResizeRight:
				return !isCustomDate;
			}

			// catch all
			return false;
		}

		protected override void OnMouseMove(MouseEventArgs e)
        {
			// default handling
			base.OnMouseMove(e);

			Cursor = GetCursor(e);
		}

		private Cursor GetCursor(MouseEventArgs e)
        {
			if (IsResizingAppointment())
				return Cursor;

			// Note: base class only shows 'resize' cursors for the currently
			// selected item but we want them for all tasks
			if (!ReadOnly)
			{
				var appt = GetAppointmentAt(e.Location.X, e.Location.Y);

				if (appt != null)
				{
					if (appt.Locked)
					{
						if (appt is CalendarExtensionItem)
							appt = GetRealAppointment(appt);

						if (appt.Locked)
							return UIExtension.AppCursor(UIExtension.AppCursorType.LockedTask);

						return UIExtension.AppCursor(UIExtension.AppCursorType.NoDrag);
					}

					var taskItem = (appt as TaskItem);

					if ((taskItem != null) && taskItem.IconRect.Contains(e.Location))
						return UIExtension.HandCursor();

					var mode = GetMode(appt, e.Location);

					if (!CanModifyAppointmentDates(appt, mode))
						return UIExtension.AppCursor(UIExtension.AppCursorType.NoDrag);

					// Same as Calendar.SelectionTool
					switch (mode)
					{
						case Calendar.SelectionTool.Mode.ResizeBottom:
						case Calendar.SelectionTool.Mode.ResizeTop:
							return Cursors.SizeNS;

						case Calendar.SelectionTool.Mode.ResizeLeft:
						case Calendar.SelectionTool.Mode.ResizeRight:
							return Cursors.SizeWE;

						case Calendar.SelectionTool.Mode.Move:
							// default cursor below
							break;
					}
				}
			}

			// All else
			return Cursors.Default;
		}

		public new int SlotsPerHour
		{
			get
			{
				return base.SlotsPerHour;
			}
			set
			{
				// Must be sensible values
				if (IsValidSlotsPerHour(value))
				{
					// If we're increasing the number of slots we force a 
					// recalculation of the min slot height else we just validate it
					if (value > base.SlotsPerHour)
					{
						minSlotHeight = m_UserMinSlotHeight;
					}
					base.SlotsPerHour = value;

					ValidateMinSlotHeight();
					AdjustVScrollbar();
				}
			}
		}

		public int MinSlotHeight
		{
			get { return m_UserMinSlotHeight; }
			set
			{
				if (value != m_UserMinSlotHeight)
				{
					m_UserMinSlotHeight = value;
					minSlotHeight = m_UserMinSlotHeight;

					ValidateMinSlotHeight();
					AdjustVScrollbar();
					Invalidate();
				}
			}

		}

		protected void ValidateMinSlotHeight()
		{
			using (var g = Graphics.FromHwnd(this.Handle))
			{
				int minHourHeight = (int)g.MeasureString("0", Renderer.HourFont).Height;

				if ((minSlotHeight * SlotsPerHour) < minHourHeight)
					minSlotHeight = ((minHourHeight / SlotsPerHour) + 1);

				if (SlotHeight < minSlotHeight)
					SlotHeight = minSlotHeight;
			}
		}
	}
}
