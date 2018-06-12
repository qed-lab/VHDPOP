; Full stomach domain designed by Yuanyuan.

(define (domain full-stomach)
  (:requirements :strips :decompositions :equality)
  (:types person location physob)
  (:constants (Food - physob)
          (Ingredients - physob)
          (Car - physob)
          (Market - location)
          (Restaurant - location))
  (:predicates (at ?p - person ?l - location)
          (at ?o - physob ?l - location)
          (has ?l - location ?o - physob)
          (owns ?p - person ?o - pyhsob)
          (full ?p - person))

  (:action full-stomach
	   :parameters (?p - person ?il - location ?el - location)
	   :precondition (and (at ?p ?il))
	   :effect (and (at ?p ?el)
		  (full ?p))
     :composite t)

  (:action transport
	   :parameters (?p - person ?il - location ?el - location)
	   :precondition (and (at ?p ?il)
			      (not (= ?il ?el)))
	   :effect (at ?p ?el)
     :composite t)

  (:action drive
     :parameters (?p -person ?il - location ?el - location)
     :precondition (and (at ?p ?il)
        (at Car ?il)
        (not (= ?il ?el)))
	   :effect (and (at ?p ?el)
        (at Car ?el))
     :composite f)

  (:action take-bus
    :parameters (?p -person ?il - location ?el - location)
    :precondition (and (at ?p ?il)
           (not (= ?il ?el)))
   	:effect (at ?p ?el)
    :composite f)

  (:action eat
    :parameters (?p - person)
    :precondition (owns ?p Food)
    :effect (and (not (owns ?p Food))
        (full ?p))
    :composite f)

  (:action cook
    :parameters (?p - person ?l - location)
    :precondition (and (owns ?p Ingredients)
        (at ?p ?l)
        (not (= ?l Market))
        (not (= ?l Restaurant)))
    :effect (and (not (owns ?p Ingredients))
        (owns ?p Food))
    :composite f)

  (:action buy
    :parameters (?p - person ?l - location ?o - physob)
    :precondition (and (at ?p ?l)
        (has ?l ?o)
    :effect (owns ?p ?o)
    :composite f)

  (:decomposition full-stomach
    :name full-by-cooking
    :parameters (?p - person ?il - location ?el - location)
    :steps ((step1(transport ?p ?il Market))
        (step2(buy ?p Market Ingredients))
        (step3(transport ?p Market ?el))
        (step4(cook ?p ?el))
        (step5(eat ?p)))
    :links ((step1(at ?p Market)step2)
        (step3(at ?p ?el)step4)
        (step2(owns ?p Ingredients)step4)
        (step4(owns ?p Food)step5)))

  (:decomposition full-stomach
    :name full-by-eating-out
    :parameters (?p - person ?il - location ?el - location)
    :steps ((step1(transport ?p ?il Restaurant))
        (step2(buy ?p Restaurant Food))
        (step3(eat ?p))
        (step4(transport ?p Restaurant ?el)))
    :links ((step1(at ?p Restaurant)step2)
        (step2(owns ?p Food)step3)
        (step2(at ?p Restaurant)step4)))

  (:decomposition transport
    :name transport-by-driving
    :parameters (?p - person ?il - location ?el - location)
    :steps ((step1(drive ?p ?il ?el))))

  (:decomposition transport
    :name transport-by-bus
    :parameters (?p - person ?il - location ?el - location)
    :steps ((step1(take-bus ?p ?il ?el))))
)
