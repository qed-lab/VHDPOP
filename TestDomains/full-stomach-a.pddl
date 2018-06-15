; A full stomach domain problem example.

(define (problem full-stomach-a)
  (:domain full-stomach)
  (:objects yuanyuan - person home - location)
  (:init (at yuanyuan home)
        (has Restaurant Food)
        (has Market Ingredients))
  (:goal (and (at yuanyuan home)
	      (full yuanyuan)
	      )))
