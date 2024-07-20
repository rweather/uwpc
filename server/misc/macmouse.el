;;;  macmouse.el (Version: 2.0)

;;;  Copyright (C) Gregory S. Lauer (glauer@bbn), 1985. 
;;;	Please send suggestions and corrections to the above address.
;;;
;;;  This file contains macmouse, a GNU Emacs mouse package for UW.


;;
;; GNU Emacs is distributed in the hope that it will be useful,
;; but without any warranty.  No author or distributor
;; accepts responsibility to anyone for the consequences of using it
;; or for whether it serves any particular purpose or works at all,
;; unless he says so in writing.

;; Everyone is granted permission to copy, modify and redistribute
;; GNU Emacs, but only under the conditions described in the
;; document "GNU Emacs copying permission notice".   An exact copy
;; of the document is supposed to have been given to you along with
;; GNU Emacs so that you can know how you may redistribute it all.
;; It should be in a file named COPYING.  Among other things, the
;; copyright notice and this notice must be preserved on all copies.


;;;  Original version for Gosling emacs by Chris Kent, Purdue University 1985.
;;;  Modified by Gregory Lauer, BBN, Novemeber 1985.
;
;
;
; Macmouse provides the following features:
;  Up or down mouse button in a window selects that window
;
;  A scroll bar/thumbing area for each window with the following features:
;       the mode lines are horizontal scroll bars
;           (running from rightmost column to under leftmost column)
;       the unused right window bar and the dividing lines between
;           windows are vertical scroll bars
;           (running from top of window THRU modeline
;   for vertical scroll bars:
; 	click at line 1 does previous page
;	click at last line does next page
; 	click anywhere else "thumbs" to the relative portion of the buffer.
; 	shift-click at line 1 scrolls one line down
; 	shift-click at last line scrolls one line up
; 	shift-click elsewhere moves line to top of window
; 	option-shift-click elsewhere moves line to bottom of window
;   for horizontal scroll bars:
;       click at column 1 does scroll right one window width
;       click at last column does scroll left one window width
;       click anywhere else moves to that "percent" of the buffer width
;       shift-click at column 1 scrolls one column right
;       shift-click at last column scrolls one column left
;       shift-click elsewhere moves column to right of window
;       option-shift-click elsewhere moves column to left of window
;
; There is also basic positioning and kill-buffer support:
; 	click in a buffer moves dot there and selects that buffer
; 	drag copies the dragged region to the kill buffer
; 	shift-drag deletes the dragged region to the kill buffer
;
;   It is possible to use the scrolling and thumbing area to make the region
;   larger than a single screen; just click, scroll, release. Make sure
;   that the last scroll is just a down event; the up must be in the buffer.
;   The last mouse position is remembered for each different buffer (not
;   window), and thus you can start a drag in one buffer, select another,
;   go back to the first buffer, etc.
;
; 	option-click yanks from the kill buffer
; 	option-shift-click similarly yanks from a named buffer.
; 

(defconst mouse-max-x 95 "Maximum UW column returned on mouse click")
(defconst mouse-max-y 95 "Maximum UW row returned on mouse click")

(make-variable-buffer-local 'mouse-last-x) ; x of last event
(set-default 'mouse-last-x 0)

(make-variable-buffer-local 'mouse-last-y) ; y of last event
(set-default 'mouse-last-y 0)

(make-variable-buffer-local 'mouse-last-b) ; buttons at last event
(set-default 'mouse-last-b 0)

(make-variable-buffer-local 'mouse-last-dot) ; dot after last event
(set-default 'mouse-last-dot 0)

(make-variable-buffer-local 'scrolling-p)
(set-default 'scrolling-p nil)

(defun move-mac-cursor ()
  (interactive)
  (let (savest b x y up down lock shift option command)
    (setq savest stack-trace-on-error)
    (setq stack-trace-on-error nil)
					; decode everything
    (setq y (- (read-char) 32))
    (setq x (- (read-char) 32))
    (setq b (- (read-char) 32))
    (setq command (< 0 (logand b 1)))	; command key
    (setq shift (< 0 (logand b 2)))	; shift
    (setq lock (< 0 (logand b 4)))	; caps-lock
    (setq option (< 0 (logand b 8)))	; option
    (setq down (< 0 (logand b 16)))	; mouse down
    (setq up (< 0 (logand b 32)))	; mouse up
    (condition-case ()
	(progn
	  (select-window-containing-x-and-y x y) ; side-effect sets scrolling-p
	  (if scrolling-p
	      (mouse-scroll-region b x y)
	    (progn
	      (move-to-window-x-y x y)	; move cursor to mouse-dot always
	      (if down (setq mouse-last-dot (dot)))
	      (mouse-edit-action))))
      (error (message "Click not in selectable window")
	     (sit-for 1)
	     (message "")))
    (setq stack-trace-on-error savest)
    (if down
	(progn 
	  (setq mouse-last-x x)
	  (setq mouse-last-y y)
	  (setq mouse-last-b b))
      (progn 
	(setq mouse-last-x 0)
	(setq mouse-last-y 0)
	(setq mouse-last-b 0)))))

(defun mouse-edit-action ()
                                ;marking and editing actions on buttons:
				;   if no movement, nothing.
				;   if movement, save mouse-last-dot,
				;      and edit.
				; editing (on upstrokes):
				;   unmodified, copy to kill buffer.
				;   SHIFTed, delete (cut) to kill buffer.
				; 
				; option-click yanks from kill buffer; 
				; shift-option-click from named buffer.
  (let ((fun (get 'mouse-function b)))
    (if fun (apply fun nil))))


    ; individual button bindings
    ; generally will only need up mouse button: mouse-last-dot
    ; is saved automatically on down mouse button

; only need to define functions for keys that get used

(put 'mouse-function 32			; up
     '(lambda ()
     	(if (and (not (mouse-click-p))
		 (not scrolling-p))
	    (copy-region-as-kill (dot) mouse-last-dot))))

(put 'mouse-function 34			; up/shift
     '(lambda ()
     	(if (and (not (mouse-click-p))
		 (not scrolling-p))
		   (kill-region (dot) mouse-last-dot))))

(put 'mouse-function 40			; up/option
     '(lambda ()
     	(if (mouse-click-p)
	    (progn
	      (yank)
	      (exchange-dot-and-mark)))))

(put 'mouse-function 42
     '(lambda ()		; up/option/shift
	(if (mouse-click-p)
	    (insert-buffer (read-buffer "Insert contents of buffer: ")))))

(defun mouse-click-p ()
  (= (dot) mouse-last-dot))

(defun set-window-boundaries ()
  (let ((edges (window-edges)))
    (setq xl (1+ (car edges)))
    (setq yt (1+ (car (cdr edges))))
    (let ((temp (car (cdr (cdr edges)))))
      (setq xr (if (= (screen-width) temp) mouse-max-x temp)))
    (let ((temp (car (cdr (cdr (cdr edges))))))
      (setq yb (if (= (screen-height) temp) mouse-max-y temp )))))

(defun select-window-containing-x-and-y (x y)
  (let ((starting-window (selected-window)))
    (set-window-boundaries)
    (while (not (point-in-window x y))
      (other-window 1)
      (if (eq (selected-window) starting-window)
	  (error nil)
	(set-window-boundaries)))
    (if (or (= x xr) (= y yb))
	(setq scrolling-p t)
      (setq scrolling-p nil))))

(defun point-in-window (x y)
  (and (<= xl x)(<= x xr)(<= yt y)(<= y yb)))

(defun move-to-window-x-y (x y)
  (move-to-window-line (- y yt))
  (move-to-window-column (- x xl)))

(defun move-to-window-column (x)
  (move-to-column (+ (max 0 (- (window-hscroll) 1)) x)))

(defun mouse-scroll-region (b x y)
  (if down
      (if shift
	  (do-lines b x y)
	(do-pages b x y)))
  (if (and up
	   (or (/= x mouse-last-x)
	       (/= y mouse-last-y)))
      (if shift
	  (do-lines b x y)
	(do-pages b x y))))

(defun do-lines (b x y)			; fine control over lines
  (if (= x xr)
      (cond ((= y yt)(scroll-down 1))
	    ((= y yb)(scroll-up 1))
	    (t (if option
		   (scroll-down (- yb y 1))
		 (scroll-up (- y yt))))))
  (if (and (= y yb) (/= x xr))
      (cond ((<= x xl)(scroll-right 1))
	    ((>= x (1- xr))(scroll-left 1))
	    (t (if option
		   (move-column-right x)
		 (move-column-left x))))))

(defun move-column-left (x)		;need to mess about a bit because
  (scroll-left				;first scroll left of 1 just writes
   (if (= (window-hscroll) 0)		;a column of $s in column 1
       (- x xl)
     (- x xl 1))))

(defun move-column-right (x)
  (scroll-right (- xr x 2)))


(defun do-pages (b x y)			; large motions via pages and thumbing
  (if (= x xr)
      (cond ((= y yt)(scroll-down nil))
	    ((= y yb)(scroll-up nil))
	    (t (goto-percent (/ (* (- y yt 1) 100)
				(- yb yt 2))))))
  (if (and (= y yb)(/= x xr))
      (cond ((<= x xl)(scroll-right (- (window-width)
				       next-screen-context-lines)))
	    ((>= x (1- xr))(scroll-left (- (window-width)
					   next-screen-context-lines)))
	    (t (goto-horizontal-percent (/ (* (- x xl 1) 100)
					   (- xr xl 2)))))))

(defun goto-percent (p)
  (goto-char (/ (* (- (dot-max) (dot-min)) p) 100)))

(defun goto-horizontal-percent (p)	;try to put this percent of columns
  (let ((window-offset (window-hscroll));in the center column of the window
	delta)				;unless that would move the first or
    (setq delta				;last column past the window edge
	  (- window-offset
	     (min (max 0 (- (/ (* (screen-width) p) 100)
			    (/ (- xr xl) 2)))
		  (- (screen-width) (- xr xl)))))
    (scroll-right delta)))

    
(global-set-key "\em" 'move-mac-cursor)
