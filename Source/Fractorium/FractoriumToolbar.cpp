#include "FractoriumPch.h"
#include "Fractorium.h"

/// <summary>
/// Initialize the toolbar UI.
/// </summary>
void Fractorium::InitToolbarUI()
{
	auto clGroup = new QActionGroup(this);
	clGroup->addAction(ui.ActionCpu);
	clGroup->addAction(ui.ActionCL);
	
	auto spGroup = new QActionGroup(this);
	spGroup->addAction(ui.ActionSP);
	spGroup->addAction(ui.ActionDP);

	SyncOptionsToToolbar();
	connect(ui.ActionCpu, SIGNAL(triggered(bool)), this, SLOT(OnActionCpu(bool)), Qt::QueuedConnection);
	connect(ui.ActionCL,  SIGNAL(triggered(bool)), this, SLOT(OnActionCL(bool)),  Qt::QueuedConnection);
	connect(ui.ActionSP,  SIGNAL(triggered(bool)), this, SLOT(OnActionSP(bool)),  Qt::QueuedConnection);
	connect(ui.ActionDP,  SIGNAL(triggered(bool)), this, SLOT(OnActionDP(bool)),  Qt::QueuedConnection);
}

/// <summary>
/// Called when the CPU render option on the toolbar is clicked.
/// </summary>
/// <param name="checked">Check state, action only taken if true.</param>
void Fractorium::OnActionCpu(bool checked)
{
	if (checked && m_Settings->OpenCL())
	{
		m_Settings->OpenCL(false);
		ShutdownAndRecreateFromOptions();
	}
}

/// <summary>
/// Called when the OpenCL render option on the toolbar is clicked.
/// </summary>
/// <param name="checked">Check state, action only taken if true.</param>
void Fractorium::OnActionCL(bool checked)
{
	if (checked && !m_Settings->OpenCL())
	{
		m_Settings->OpenCL(true);
		ShutdownAndRecreateFromOptions();
	}
}

/// <summary>
/// Called when the single precision render option on the toolbar is clicked.
/// </summary>
/// <param name="checked">Check state, action only taken if true.</param>
void Fractorium::OnActionSP(bool checked)
{
	if (checked && m_Settings->Double())
	{
		m_Settings->Double(false);
		ShutdownAndRecreateFromOptions();
	}
}

/// <summary>
/// Called when the double precision render option on the toolbar is clicked.
/// </summary>
/// <param name="checked">Check state, action only taken if true.</param>
void Fractorium::OnActionDP(bool checked)
{
	if (checked && !m_Settings->Double())
	{
		m_Settings->Double(true);
		ShutdownAndRecreateFromOptions();
	}
}

/// <summary>
/// Sync options data to the check state of the toolbar buttons.
/// This does not trigger a clicked() event.
/// </summary>
void Fractorium::SyncOptionsToToolbar()
{
	if (m_Settings->OpenCL())
	{
		ui.ActionCpu->setChecked(false);
		ui.ActionCL->setChecked(true);
	}
	else
	{
		ui.ActionCpu->setChecked(true);
		ui.ActionCL->setChecked(false);
	}

	if (m_Settings->Double())
	{
		ui.ActionSP->setChecked(false);
		ui.ActionDP->setChecked(true);
	}
	else
	{
		ui.ActionSP->setChecked(true);
		ui.ActionDP->setChecked(false);
	}
}