#ifndef IMAGEVIEW_H_
#define IMAGEVIEW_H_

#include <QGraphicsView>

class ImageView : public QGraphicsView {
protected:
	void resizeEvent(QResizeEvent *event) override;

public:
	ImageView(QWidget *parent = 0);
};

#endif	// IMAGEVIEW_H_
