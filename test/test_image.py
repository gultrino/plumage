import unittest
import Tkinter

import support

class PhotoImageTest(unittest.TestCase):

    def test_imgcreation(self):
        self.failUnlessRaises(Tkinter.TclError, Tkinter.PhotoImage,
                file=support.sample_img("test1.ppm"), format="bad_one")

        self.failUnlessRaises(Tkinter.TclError, Tkinter.PhotoImage,
                file=support.sample_img("test1.ppm"), format="gif")

        self.failUnlessRaises(Tkinter.TclError, Tkinter.PhotoImage,
                file="no_file")

        img = Tkinter.PhotoImage(file=support.sample_img("test1.ppm"))
        self.failUnlessEqual(float(img['gamma']), 1.0)
        self.failUnlessEqual(img.type(), 'photo')

    def test_data(self):
        img = Tkinter.PhotoImage(file=support.sample_img("test1.gif"))
        data = img.data(from_coords=(0, 0, 5, 6), grayscale=True)

        x = tuple(data)
        self.failUnlessEqual(len(x), 6)
        self.failUnlessEqual(len(x[0]), 5)

    def test_blank(self):
        img = Tkinter.PhotoImage(file=support.sample_img("test1.pgm"))
        img.blank()
        for pixel in tuple(img.data())[0]:
            self.failUnlessEqual(pixel, '#000000')

    def test_putpixels(self):
        img = Tkinter.PhotoImage(file=support.sample_img("test1.pgm"))
        img.blank()

        img.put_pixels((("red", "blue", "green"), ("green", "red", "blue")))
        data = tuple(img.data(from_coords=(0, 0, 3, 2)))
        self.failUnlessEqual(data,
                (['#ff0000', '#0000ff', '#00ff00'],
                    ['#00ff00', '#ff0000', '#0000ff']))

    def test_transparency(self):
        img = Tkinter.PhotoImage(file=support.sample_img("test1.gif"))
        boolval = img.transparency_get(0, 0)
        img.transparency_set(0, 0)
        self.failUnlessEqual(img.transparency_get(0, 0), not boolval)


class BitmapImageTest(unittest.TestCase):

    def test_bitmap(self):
        img = Tkinter.BitmapImage(file=support.sample_img("test1.bmp"))

        self.failIf(img.inuse())
        self.failUnlessEqual(img.type(), 'bitmap')
        img_name = str(img)
        self.failUnless(img_name in Tkinter.image_names())
        del img
        self.failIf(img_name in Tkinter.image_names())


class ImageTest(unittest.TestCase):

    def test_rettype(self):
        img = Tkinter.PhotoImage(file=support.sample_img("test1.ppm"))

        width = img.width()
        height = img.height()
        self.failUnless(isinstance(width, int))
        self.failUnless(isinstance(height, int))
        self.failUnlessEqual(width, height)
        self.failUnlessEqual(width, 420)


class ImageFunctionsTest(unittest.TestCase):

    def test_imagetypes(self):
        types = Tkinter.image_types()
        self.failUnless(isinstance(types, tuple))

    def test_imagenames(self):
        img = Tkinter.BitmapImage(file=support.sample_img("test1.bmp"))
        self.failUnless(isinstance(Tkinter.image_names(), tuple))
        del img
        self.failUnlessEqual(Tkinter.image_names(), ())


def test_main():
    support.run(PhotoImageTest, BitmapImageTest, ImageTest, ImageFunctionsTest)

if __name__ == "__main__":
    test_main()
