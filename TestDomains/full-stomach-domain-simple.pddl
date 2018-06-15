; Full stomach domain designed by Yuanyuan.

(define (domain full-stomach)
  (:requirements :strips :equality)
  (:types person location physob - object)
  (:constants Food Ingredients Car - physob Market Restaurant - location)
  (:predicates (at ?p - person ?l - location)
          (has ?l - location ?o - physob)
          (locates ?l - location ?o - physob)
          (owns ?p - person ?o - physob)
          (full ?p - person))

  (:action drive
     :parameters (?p -person ?il - location ?el - location)
     :precondition (and (at ?p ?il)
        (locates ?il Car)
        (owns ?p Car)
        (not (= ?il ?el)))
     :effect (and (at ?p ?el)
        (locates ?el Car)
        (not (locates ?il Car))
        (not (at ?p ?il))))

  (:action take-bus
    :parameters (?p -person ?il - location ?el - location)
    :precondition (and (at ?p ?il)
           (not (= ?il ?el)))
   	:effect (and (at ?p ?el)
   	        (not (at ?p ?il))))

  (:action eat
    :parameters (?p - person)
    :precondition (owns ?p Food)
    :effect (and (not (owns ?p Food))
        (full ?p)))

  (:action cook
    :parameters (?p - person ?l - location)
    :precondition (and (owns ?p Ingredients)
        (at ?p ?l)
        (not (= ?l Market))
        (not (= ?l Restaurant)))
    :effect (and (not (owns ?p Ingredients))
        (owns ?p Food)))

  (:action buy
    :parameters (?p - person ?l - location ?o - physob)
    :precondition (and (at ?p ?l)
        (has ?l ?o))
    :effect (owns ?p ?o))
)
