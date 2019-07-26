#include "AboutDialog.h"

/**

- QGridLayout
	- QHBoxLayout
		- QSpacerItem
		- QLabel
		- QSpacerItem
	- QLabel (Name/Version/ReleaseDate)
	- QGroupBox (Description)
		- QLable
	- QGroupBox (Copy Right)
		- QLable

 */
AboutDialog::AboutDialog(QWidget *parent) :
	QDialog(parent)
{
	QFont font;
	font.setPointSize(9);
	//font.setFamily(QStringLiteral("Ping Hei"));
	font.setFamily(QStringLiteral("Comic Sans MS"));
	this->setFont(font);

	setWindowModality(Qt::NonModal);
	resize(400, 300);
	setWindowTitle(tr("varApplicationName"));
	setWindowIcon(QIcon(":images/appIcon_64x64.png"));
	m_gridLayout = new QGridLayout(this);

	m_horizontalLayout = new QHBoxLayout();
	m_horizontalSpacer1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	m_horizontalLayout->addItem(m_horizontalSpacer1);
	m_labelLogo = new QLabel(this);
	m_labelLogo->setMinimumSize(QSize(64, 64));
	m_labelLogo->setMaximumSize(QSize(64, 64));
	m_labelLogo->setText(QStringLiteral(""));
	m_labelLogo->setPixmap(QPixmap(QString::fromUtf8(":images/appIcon_64x64.png")));
	m_labelLogo->setScaledContents(true);
	m_labelLogo->setAlignment(Qt::AlignCenter);
	m_horizontalLayout->addWidget(m_labelLogo);
	m_horizontalSpacer2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	m_horizontalLayout->addItem(m_horizontalSpacer2);
	m_gridLayout->addLayout(m_horizontalLayout, 0, 0);

	m_labelBasicInfo = new QLabel(this);
	m_labelBasicInfo->setAlignment(Qt::AlignCenter);
	m_labelBasicInfo->setText(tr(
		"<h3><b>Application Description</b></h3>"
		"<p style='color:blue'>Version %1</p>"
		"<p style='color:blue'>Release Date: %2<p>"
		).arg("varVersion").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
		);
	m_gridLayout->addWidget(m_labelBasicInfo);

	m_gbOptInfo1 = new QGroupBox(this);
	m_gbOptInfo1->setTitle(QStringLiteral("Description"));
	m_gbOptInfo1->setAlignment(Qt::AlignCenter);
	m_gbOptInfo1->setFlat(true);
	m_gridLayoutOptInfo1 = new QGridLayout(m_gbOptInfo1);
	m_labelOptInfo1 = new QLabel(this);
	m_labelOptInfo1->setWordWrap(true);
	m_labelOptInfo1->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
	m_labelOptInfo1->setText(tr(
		"Well, please put a long description for your application here, we will show it in correct format."
		)
		);
	m_gridLayoutOptInfo1->addWidget(m_labelOptInfo1, 0, 0, 1, 1);
	m_gridLayout->addWidget(m_gbOptInfo1);

	m_gbOptInfo2 = new QGroupBox(this);
	m_gbOptInfo2->setTitle(QStringLiteral("About Author"));
	m_gbOptInfo2->setAlignment(Qt::AlignCenter);
	m_gbOptInfo2->setFlat(true);
	m_gridLayoutOptInfo2 = new QGridLayout(m_gbOptInfo2);
	m_labelOptInfo2 = new QLabel(this);
	m_labelOptInfo2->setWordWrap(true);
	m_labelOptInfo2->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
	m_labelOptInfo2->setText(tr(
		"Copyright (C) 2016 ~ 2020 SAPA"
		)
		);
	m_gridLayoutOptInfo2->addWidget(m_labelOptInfo2, 0, 0, 1, 1);
	m_gridLayout->addWidget(m_gbOptInfo2);

	//m_gridLayout->setColumnStretch(0, 1);
	m_gridLayout->setRowStretch(0, 2);
	m_gridLayout->setRowStretch(1, 1);
	m_gridLayout->setRowStretch(1, 1);
	m_gridLayout->setRowStretch(1, 1);

	show();
}

AboutDialog::~AboutDialog()
{

}