{-# LANGUAGE FlexibleInstances #-}
import Data.Bits
import Data.Ix
import Data.Function
import Data.Word
import Test.QuickCheck

packBits :: FiniteBits b => [Bool] -> b
packBits xs = foldr (.|.) zeroBits (zipWith shiftL (map f xs) [0..]) where
    f True = bit 0
    f False = zeroBits

unpackBits :: FiniteBits b => b -> [Bool]
unpackBits x = [testBit x i | i <- [0..finiteBitSize x]]

checkUnpackPackId = do
    let i = packBits . unpackBits
    quickCheck (\x -> i x == id (x :: Word64))

mapBits :: FiniteBits b => (Bool -> Bool -> Bool) -> b -> b -> b
mapBits f x y = packBits (zipWith f (unpackBits x) (unpackBits y))

truthTableToFunction :: (Bool, Bool, Bool, Bool) -> (Bool -> Bool -> Bool)
truthTableToFunction (a,b,c,d) = f where
    f False False = a
    f False  True = b
    f  True False = c
    f  True  True = d

functionToTruthTable f = (a,b,c,d) where
    a = f False False
    b = f False  True
    c = f  True False
    d = f  True  True

allGates = map truthTableToFunction (range ((False,False,False,False),(True,True,True,True)))
allGatePairs = [(f,g) | f <- allGates, g <- allGates]

mapReduce f g b x y = foldr g b (zipWith f (unpackBits x) (unpackBits y))
bruteFunction = [(f,g,b) | (f,g) <- allGatePairs, b <- [False,True]]

bruteMapReduce = do
    let tablify (f,g,b) = (functionToTruthTable f, functionToTruthTable g, b)
    let bruteNumber = [(x,y) | x <- d, y <- d] where d = [minBound::Word8 .. maxBound]
    mapM (print . tablify) (filter (\(f,g,b) -> all (\(x,y) -> (x < y) == mapReduce f g b x y) bruteNumber) bruteFunction)

instance Show (Bool -> Bool -> Bool) where
    show = aux . functionToTruthTable where
        aux (False, True, False, False) = "(<)"
        aux (False, True, True, True) = "(||)"
        aux (False, False, False, True) = "(&&)"
        aux (True, False, False, True) = "xnor"
        aux x = show x

instance Eq (Bool -> Bool -> Bool) where (==) = (==) `on` functionToTruthTable

data Gate = Constant Bool | Input Bool Int | Function (Bool->Bool->Bool) Gate Gate deriving (Eq, Show)

mkManualLessThan size = output where
    primitive f i = Function f (Input False i) (Input True i)
    lessthans = map (primitive (<)) [size-1,size-2..0]
    xnors = Constant True : map (primitive ((not .) . xor)) [size-1,size-2..0]
    intermediate1 = Constant True : zipWith (Function (&&)) intermediate1 (tail xnors)
    intermediate2 = zipWith3 (\x y z -> (Function (&&) x (Function (&&) y z))) lessthans xnors intermediate1
    output = foldr (Function (||)) (Constant False) intermediate2

eval (Constant b) = (\_ _ -> b)
eval (Input False i) = (\x y -> testBit x i)
eval (Input True i) = (\x y -> testBit y i)
eval (Function f g1 g2) = (\x y -> f (eval g1 x y) (eval g2 x y))

optimize :: Gate -> Gate
optimize (Function f g1 g2) = aux (functionToTruthTable f) g1 g2 where
    andT = functionToTruthTable (&&)
    orT = functionToTruthTable (||)
    aux t g1 (Constant True) | t == andT = optimize g1
    aux t (Constant True) g2 | t == andT = optimize g2
    aux t g1 (Constant False) | t == orT = optimize g1
    aux t (Constant False) g2 | t == orT = optimize g2
    aux _ g1 g2 = Function f (optimize g1) (optimize g2)
optimize x = x

fixConverge f x = let y = f x in if y == x then y else fixConverge f y

counterExamples n = filter (\(_,_,a,b) -> a /= b) [(x :: Word64,y,eval circuit x y,x < y) | x <- [0..2^n-1], y <- [0..2^n-1]] where
    circuit = mkManualLessThan n
counterExamples' n = filter (\(_,_,a,b) -> a /= b) [(x :: Word64,y,eval circuit x y,x < y) | x <- [0..2^n-1], y <- [0..2^n-1]] where
    circuit = fixConverge optimize $ mkManualLessThan n


main = do
    mapM (print . counterExamples) [0..8]
    putStrLn "-----"
    mapM (print . counterExamples') [0..8]
    putStrLn "-----"
    verboseCheck (\x y -> eval (mkManualLessThan 64) (x :: Word64) y == (x < y))
